#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "ChordcraftAudioProcessor.h"
#include "ChordArrangement.h"
#include "ChordDatabase.h"
#include "PatternDatabase.h"
#include <cmath>

//==============================================================================
ChordcraftAudioProcessor::ChordcraftAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Load General MIDI SoundFont from memory
    tsfInstance = tsf_load_memory (BinaryData::general_midi_sf2, BinaryData::general_midi_sf2Size);
    if (tsfInstance != nullptr)
    {
        tsf_set_output (tsfInstance, TSF_STEREO_INTERLEAVED, 44100, 0.0f);
        
        // Initialize all channels to preset 0 (Acoustic Grand Piano)
        // Except channel 9 (Drums, MIDI Ch 10) which uses flag_mididrums = 1
        for (int ch = 0; ch < 16; ++ch)
        {
            tsf_channel_set_presetnumber (tsfInstance, ch, 0, (ch == 9 ? 1 : 0));
        }
    }
}

ChordcraftAudioProcessor::~ChordcraftAudioProcessor()
{
    {
        const juce::ScopedLock sl (midiOutputLock);
        activeMidiOutputs.clear();
    }

    if (tsfInstance != nullptr)
    {
        tsf_close (tsfInstance);
        tsfInstance = nullptr;
    }
}

//==============================================================================
void ChordcraftAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateLocal = sampleRate;
    currentSamplePosition = 0;
    updateSequencerTiming();
    
    if (tsfInstance != nullptr)
    {
        tsf_set_output (tsfInstance, TSF_STEREO_INTERLEAVED, static_cast<int> (sampleRate), 0.0f);
    }
    
    // Allocate stack de-interleave buffer (2 channels) - preallocate to at least 4096 frames to handle mobile fluctuations
    interpBuffer.resize (std::max (samplesPerBlock, 4096) * 2, 0.0f);
}

void ChordcraftAudioProcessor::releaseResources()
{
    if (tsfInstance != nullptr)
    {
        tsf_note_off_all (tsfInstance);
    }
}

void ChordcraftAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 1. Dispatch lock-free queue commands
    processQueueInstructions();

    // 2. Run the GM pattern sequencer
    runSequencer (buffer.getNumSamples(), midiMessages);

    // 2.5. Send real-time MIDI to open hardware/virtual output ports
    {
        const juce::ScopedTryLock sl (midiOutputLock);
        if (sl.isLocked())
        {
            for (auto* out : activeMidiOutputs)
            {
                out->sendBlockOfMessages (midiMessages, juce::Time::getMillisecondCounter(), sampleRateLocal);
            }
        }
    }

    // 3. Render TinySoundFont vector audio directly
    if (tsfInstance != nullptr)
    {
        int numSamples = buffer.getNumSamples();
        
        // Safety check to ensure we always have enough space in the de-interleave buffer.
        if (interpBuffer.size() < static_cast<size_t> (numSamples * 2))
        {
            interpBuffer.resize (numSamples * 2, 0.0f);
        }
        
        // Render stereo interleaved into temp buffer
        tsf_render_float (tsfInstance, interpBuffer.data(), numSamples, 0);
        
        auto* leftChannel = buffer.getWritePointer (0);
        auto* rightChannel = buffer.getWritePointer (1);
        
        // De-interleave directly into the output channels and apply soft-clipping
        for (int i = 0; i < numSamples; ++i)
        {
            leftChannel[i] = std::tanh (interpBuffer[i * 2]);
            rightChannel[i] = std::tanh (interpBuffer[i * 2 + 1]);
        }
    }
}

//==============================================================================
void ChordcraftAudioProcessor::queueInstruction (const PlaybackInstruction& instruction)
{
    instructionQueue.push (instruction);
}

void ChordcraftAudioProcessor::auditionNoteOn (int midiNote, int velocity)
{
    PlaybackInstruction inst;
    inst.type = PlaybackInstruction::NoteOn;
    inst.midiNote = midiNote;
    inst.velocity = velocity;
    queueInstruction (inst);
}

void ChordcraftAudioProcessor::auditionNoteOff (int midiNote)
{
    PlaybackInstruction inst;
    inst.type = PlaybackInstruction::NoteOff;
    inst.midiNote = midiNote;
    queueInstruction (inst);
}

void ChordcraftAudioProcessor::clearAudition()
{
    PlaybackInstruction inst;
    inst.type = PlaybackInstruction::AllNotesOff;
    queueInstruction (inst);
}

void ChordcraftAudioProcessor::setTempo (float bpm)
{
    PlaybackInstruction inst;
    inst.type = PlaybackInstruction::SetTempo;
    inst.tempo = bpm;
    queueInstruction (inst);
}

void ChordcraftAudioProcessor::setPlayState (bool play)
{
    PlaybackInstruction inst;
    inst.type = PlaybackInstruction::SetPlayState;
    inst.isPlaying = play;
    queueInstruction (inst);
}

void ChordcraftAudioProcessor::setChannelProgram (int channel, int programNumber)
{
    PlaybackInstruction inst;
    inst.type = PlaybackInstruction::SetChannelProgram;
    inst.blockIndex = channel; // 0-15
    inst.midiNote = programNumber; // 0-127
    queueInstruction (inst);
}

void ChordcraftAudioProcessor::sendProgressionToAudioThread (const std::vector<SongSection>& sections, int activeSectionIndex)
{
    isSingleShotPreview.store (false);
    {
        const juce::ScopedLock sl (stagingLock);
        
        stagingPlaybackData.sections.clear();
        stagingPlaybackData.events.clear();
        stagingPlaybackData.totalDurationSamples = 0.0;
        
        int currentStartTick = 0;
        double currentStartSample = 0.0;
        const int ticksPerQuarterNote = 960;
        
        for (int s = 0; s < (int) sections.size(); ++s)
        {
            auto& sec = sections[s];
            int secDurationTicks = 0;
            for (auto& cb : sec.blocks)
                secDurationTicks += cb.durationBeats * ticksPerQuarterNote;
                
            if (secDurationTicks <= 0) continue;
            
            // Loop this section loopCount times
            for (int loop = 0; loop < sec.loopCount; ++loop)
            {
                PlaybackSection pSec;
                pSec.name = sec.sectionName + " (Loop " + std::to_string (loop + 1) + ")";
                pSec.startTick = currentStartTick;
                pSec.durationTicks = secDurationTicks;
                pSec.bpm = sec.bpm;
                pSec.loopCount = sec.loopCount;
                pSec.startSample = currentStartSample;
                
                // Preset volumes and programs
                for (int ch = 0; ch < 16; ++ch)
                {
                    pSec.programs[ch] = 0;
                    pSec.volumes[ch] = 0.6f;
                }
                
                // Map sections tracks
                int melodicCount = 0;
                for (int i = 0; i < (int) sec.tracks.size(); ++i)
                {
                    int channel = 0;
                    if (sec.tracks[i].isDrums)
                    {
                        channel = 9;
                    }
                    else
                    {
                        if (melodicCount == 9) melodicCount++;
                        channel = melodicCount++;
                        if (channel > 15) channel = 15;
                    }
                    pSec.programs[channel] = sec.tracks[i].gmProgramNumber;
                    pSec.volumes[channel] = sec.tracks[i].volume;
                }
                
                double secSamplesPerTick = sampleRateLocal / (16.0 * sec.bpm);
                pSec.durationSamples = secDurationTicks * secSamplesPerTick;
                
                stagingPlaybackData.sections.push_back (pSec);
                
                // Compile all sequencer events for this loop of this section
                int blockStartTick = currentStartTick;
                for (int b = 0; b < (int) sec.blocks.size(); ++b)
                {
                    auto& cb = sec.blocks[b];
                    int blockDurationTicks = cb.durationBeats * ticksPerQuarterNote;
                    
                    // Resolve MIDI notes
                    juce::Array<int> chordMidiNotes = resolveChordMidiNotes (cb);
                    
                    // Generate note events for all lanes
                    melodicCount = 0;
                    for (int laneIdx = 0; laneIdx < (int) sec.tracks.size(); ++laneIdx)
                    {
                        auto& lane = sec.tracks[laneIdx];

                        // Determine the absolute channel BEFORE skipping disabled lanes
                        int channel = 0;
                        if (lane.isDrums)
                        {
                            channel = 9;
                        }
                        else
                        {
                            if (melodicCount == 9) melodicCount++;
                            channel = melodicCount++;
                            if (channel > 15) channel = 15;
                        }

                        if (! lane.enabled || lane.patternId.isEmpty())
                            continue;
                            
                        auto* pattern = PatternDatabase::getInstance().getPatternById (lane.patternId.toStdString());
                        if (pattern == nullptr)
                            continue;
                            
                        // Round up pattern length to the nearest quarter-note (960 ticks)
                        int patternLength = 3840;
                        int maxNoteEnd = 0;
                        for (auto& n : pattern->notes)
                            maxNoteEnd = std::max (maxNoteEnd, n.tick + n.duration);
                        if (maxNoteEnd > 0)
                            patternLength = 960 * ((maxNoteEnd + 959) / 960);
                            
                        for (int offset = 0; offset < blockDurationTicks; offset += patternLength)
                        {
                            for (auto& note : pattern->notes)
                            {
                                int noteStartInBlock = offset + note.tick;
                                if (noteStartInBlock >= blockDurationTicks)
                                    continue;
                                    
                                int noteEndInBlock = std::min (blockDurationTicks, noteStartInBlock + note.duration);
                                int absStartTick = blockStartTick + noteStartInBlock;
                                int absEndTick = blockStartTick + noteEndInBlock;
                                
                                int midiNote = 60;
                                if (note.drumMidi.has_value())
                                {
                                    midiNote = note.drumMidi.value();
                                }
                                else if (note.degree.has_value())
                                {
                                    if (! chordMidiNotes.isEmpty())
                                    {
                                        int degree = note.degree.value();
                                        int chordSize = chordMidiNotes.size();
                                        int wrappedDegree = ((degree % chordSize) + chordSize) % chordSize;
                                        int octaveShift = (int)std::floor((float)degree / chordSize);
                                        midiNote = chordMidiNotes[wrappedDegree] + (octaveShift * 12);
                                        
                                        if (lane.gmProgramNumber >= 32 && lane.gmProgramNumber <= 39)
                                        {
                                            midiNote -= 24;
                                        }
                                    }
                                }
                                
                                // Clamp note range for safety
                                if (! note.drumMidi.has_value())
                                {
                                    while (midiNote < 28)  { midiNote += 12; }
                                    while (midiNote > 108) { midiNote -= 12; }
                                }
                                
                                // Generate note on
                                SequencerEvent evOn;
                                evOn.tick = absStartTick;
                                evOn.midiNote = midiNote;
                                evOn.velocity = note.velocity;
                                evOn.channel = channel;
                                evOn.isNoteOn = true;
                                stagingPlaybackData.events.push_back (evOn);
                                
                                // Generate note off
                                SequencerEvent evOff;
                                evOff.tick = absEndTick;
                                evOff.midiNote = midiNote;
                                evOff.velocity = 0;
                                evOff.channel = channel;
                                evOff.isNoteOn = false;
                                stagingPlaybackData.events.push_back (evOff);
                            }
                        }
                    }
                    blockStartTick += blockDurationTicks;
                }
                
                currentStartTick += secDurationTicks;
                currentStartSample += pSec.durationSamples;
            }
        }
        
        stagingPlaybackData.totalDurationSamples = currentStartSample;
        
        // Sort all compiled events chronologically by tick
        std::sort (stagingPlaybackData.events.begin(), stagingPlaybackData.events.end(), [](const SequencerEvent& a, const SequencerEvent& b) {
            if (a.tick == b.tick)
                return ! a.isNoteOn && b.isNoteOn; // Process note offs first at same tick
            return a.tick < b.tick;
        });
    }
    
    PlaybackInstruction swapInst;
    swapInst.type = PlaybackInstruction::SwapProgression;
    queueInstruction (swapInst);
}

//==============================================================================
void ChordcraftAudioProcessor::processQueueInstructions()
{
    PlaybackInstruction inst;
    while (instructionQueue.pop (inst))
    {
        switch (inst.type)
        {
            case PlaybackInstruction::NoteOn:
                if (tsfInstance != nullptr)
                {
                    tsf_channel_note_on (tsfInstance, 0, inst.midiNote, (float) inst.velocity / 127.0f);
                }
                break;
                
            case PlaybackInstruction::NoteOff:
                if (tsfInstance != nullptr)
                {
                    tsf_channel_note_off (tsfInstance, 0, inst.midiNote);
                }
                break;
                
            case PlaybackInstruction::AllNotesOff:
                if (tsfInstance != nullptr)
                {
                    tsf_note_off_all (tsfInstance);
                }
                break;
                
            case PlaybackInstruction::SetTempo:
                tempoLocal = inst.tempo;
                tempoAtomic.set (tempoLocal);
                updateSequencerTiming();
                break;
                
            case PlaybackInstruction::SetPlayState:
                isPlayingLocal = inst.isPlaying;
                isPlayingAtomic.set (isPlayingLocal ? 1 : 0);
                if (! isPlayingLocal)
                {
                    if (tsfInstance != nullptr)
                    {
                        tsf_note_off_all (tsfInstance);
                    }
                    currentSamplePosition = 0;
                    lastSectionIndexPlayed = -1;
                }
                break;
                


                
            case PlaybackInstruction::SetChannelProgram:
                if (tsfInstance != nullptr)
                {
                    int chan = inst.blockIndex;
                    int prog = inst.midiNote;
                    tsf_channel_set_presetnumber (tsfInstance, chan, prog, (chan == 9 ? 1 : 0));
                }
                break;

            case PlaybackInstruction::SetChannelVolume:
                if (tsfInstance != nullptr)
                {
                    int chan = inst.blockIndex;
                    float vol = inst.tempo; // using tempo field for volume float
                    tsf_channel_set_volume (tsfInstance, chan, vol);
                }
                break;

            case PlaybackInstruction::SwapProgression:
                {
                    const juce::ScopedTryLock sl (stagingLock);
                    if (sl.isLocked())
                    {
                        std::swap (activePlaybackData, stagingPlaybackData);
                        lastSectionIndexPlayed = -1;
                        
                        double totalDurationSamples = activePlaybackData.totalDurationSamples;
                        if (totalDurationSamples > 0.0)
                        {
                            if (currentSamplePosition >= totalDurationSamples)
                            {
                                currentSamplePosition = std::fmod (currentSamplePosition, totalDurationSamples);
                            }
                            else if (currentSamplePosition < 0.0)
                            {
                                currentSamplePosition = 0.0;
                            }
                        }
                        else
                        {
                            currentSamplePosition = 0.0;
                        }
                    }
                    else
                    {
                        // Staging lock is busy, re-queue to retry on next audio block
                        queueInstruction (inst);
                    }
                }
                break;
                
            default:
                break;
        }
    }
}

void ChordcraftAudioProcessor::runSequencer (int numSamples, juce::MidiBuffer& midiMessages)
{
    double totalDurationSamples = activePlaybackData.totalDurationSamples;
    if (! isPlayingLocal || totalDurationSamples <= 0.0)
        return;

    int samplesRemaining = numSamples;
    int sampleOffsetWithinBlock = 0;

    while (samplesRemaining > 0)
    {
        double samplesToBoundary = totalDurationSamples - currentSamplePosition;
        int samplesToProcess = samplesRemaining;
        bool hitsBoundary = false;

        if (samplesToBoundary <= samplesToProcess)
        {
            samplesToProcess = static_cast<int> (samplesToBoundary);
            hitsBoundary = true;
        }

        // Find the current PlaybackSection index
        int currentSecIdx = 0;
        int numSections = (int) activePlaybackData.sections.size();
        for (int i = 0; i < numSections; ++i)
        {
            auto& sec = activePlaybackData.sections[i];
            if (currentSamplePosition >= sec.startSample && currentSamplePosition < sec.startSample + sec.durationSamples)
            {
                currentSecIdx = i;
                break;
            }
            if (i == numSections - 1)
                currentSecIdx = i;
        }

        auto& currentSec = activePlaybackData.sections[currentSecIdx];
        double nextSectionBoundary = currentSec.startSample + currentSec.durationSamples;
        double samplesToNextSection = nextSectionBoundary - currentSamplePosition;

        if (samplesToNextSection < samplesToProcess)
        {
            samplesToProcess = static_cast<int> (samplesToNextSection);
            hitsBoundary = false;
            if (nextSectionBoundary >= totalDurationSamples - 0.001)
                hitsBoundary = true;
        }

        double nextSamplePosition = currentSamplePosition + samplesToProcess;

        if (samplesToProcess > 0)
        {
            double secSamplesPerTick = sampleRateLocal / (16.0 * currentSec.bpm);
            double sampleOffsetInSec = currentSamplePosition - currentSec.startSample;
            int startTick = currentSec.startTick + static_cast<int> (sampleOffsetInSec / secSamplesPerTick);
            
            double nextSampleOffsetInSec = nextSamplePosition - currentSec.startSample;
            int endTick = (nextSamplePosition >= nextSectionBoundary - 0.001) 
                          ? (currentSec.startTick + currentSec.durationTicks + 1)
                          : currentSec.startTick + static_cast<int> (nextSampleOffsetInSec / secSamplesPerTick);

            // Apply section presets if we entered a new section
            if (currentSecIdx != lastSectionIndexPlayed)
            {
                lastSectionIndexPlayed = currentSecIdx;
                playingSectionIndexAtomic.set (currentSecIdx);
                if (tsfInstance != nullptr)
                {
                    for (int ch = 0; ch < 16; ++ch)
                    {
                        tsf_channel_set_presetnumber (tsfInstance, ch, currentSec.programs[ch], (ch == 9 ? 1 : 0));
                        tsf_channel_set_volume (tsfInstance, ch, currentSec.volumes[ch]);
                        
                        // Expose states to the buffer so the MIDI Export captures instruments and levels
                        midiMessages.addEvent (juce::MidiMessage::programChange (ch + 1, currentSec.programs[ch]), sampleOffsetWithinBlock);
                        midiMessages.addEvent (juce::MidiMessage::controllerEvent (ch + 1, 7, static_cast<int> (currentSec.volumes[ch] * 127.0f)), sampleOffsetWithinBlock);
                    }
                }
            }

            // Sync playhead atomic relative to the start of this section
            if (sampleOffsetWithinBlock == 0)
            {
                int localTick = startTick - currentSec.startTick;
                playheadTickAtomic.set (localTick);
            }

            // Pass 1: Note-Off events first
            for (auto& ev : activePlaybackData.events)
            {
                if (! ev.isNoteOn && ev.tick >= startTick && ev.tick < endTick)
                {
                    double tickOffsetInSec = ev.tick - currentSec.startTick;
                    double samplePosOfEvent = currentSec.startSample + (tickOffsetInSec * secSamplesPerTick);
                    double eventOffsetInBlock = samplePosOfEvent - currentSamplePosition;
                    int totalOffset = sampleOffsetWithinBlock + static_cast<int> (eventOffsetInBlock);
                    totalOffset = juce::jlimit (0, numSamples - 1, totalOffset);

                    if (tsfInstance != nullptr)
                    {
                        tsf_channel_note_off (tsfInstance, ev.channel, ev.midiNote);
                    }
                    midiMessages.addEvent (juce::MidiMessage::noteOff (ev.channel + 1, ev.midiNote), totalOffset);
                }
            }

            // Pass 2: Note-On events
            for (auto& ev : activePlaybackData.events)
            {
                if (ev.isNoteOn && ev.tick >= startTick && ev.tick < endTick)
                {
                    double tickOffsetInSec = ev.tick - currentSec.startTick;
                    double samplePosOfEvent = currentSec.startSample + (tickOffsetInSec * secSamplesPerTick);
                    double eventOffsetInBlock = samplePosOfEvent - currentSamplePosition;
                    int totalOffset = sampleOffsetWithinBlock + static_cast<int> (eventOffsetInBlock);
                    totalOffset = juce::jlimit (0, numSamples - 1, totalOffset);

                    if (tsfInstance != nullptr)
                    {
                        tsf_channel_note_on (tsfInstance, ev.channel, ev.midiNote, (float) ev.velocity / 127.0f);
                    }
                    midiMessages.addEvent (juce::MidiMessage::noteOn (ev.channel + 1, ev.midiNote, (float) ev.velocity / 127.0f), totalOffset);
                }
            }

            currentSamplePosition = nextSamplePosition;
            sampleOffsetWithinBlock += samplesToProcess;
            samplesRemaining -= samplesToProcess;
        }
        else
        {
            hitsBoundary = true;
        }

        if (hitsBoundary)
        {
            currentSamplePosition = nextSamplePosition - totalDurationSamples;
            if (currentSamplePosition < 0.0) currentSamplePosition = 0.0;

            if (isSingleShotPreview.load())
            {
                isPlayingLocal = false;
                isPlayingAtomic.set (0);
                isSingleShotPreview.store (false);

                if (tsfInstance != nullptr)
                    tsf_note_off_all (tsfInstance);

                for (int ch = 1; ch <= 16; ++ch)
                    midiMessages.addEvent (juce::MidiMessage::allNotesOff (ch), sampleOffsetWithinBlock);

                break;
            }
        }
    }
}

void ChordcraftAudioProcessor::updateSequencerTiming()
{
    if (tempoLocal > 0.0f)
        samplesPerTick = sampleRateLocal / (16.0 * tempoLocal);
    else
        samplesPerTick = 0.0;
}

void ChordcraftAudioProcessor::updateActiveMidiOutputs()
{
    auto devices = juce::MidiOutput::getAvailableDevices();
    
    juce::OwnedArray<juce::MidiOutput> newOutputs;
    for (auto& d : devices)
    {
        if (auto out = juce::MidiOutput::openDevice (d.identifier))
        {
            out->startBackgroundThread();
            newOutputs.add (out.release());
        }
    }
    
    {
        const juce::ScopedLock sl (midiOutputLock);
        activeMidiOutputs.swapWith (newOutputs);
    }
}

void ChordcraftAudioProcessor::playChordPreview (const ChordBlock& cb, const std::vector<TrackLane>& lanes)
{
    setPlayState (false);
    isSingleShotPreview.store (true);
    
    {
        const juce::ScopedLock sl (stagingLock);
        
        stagingPlaybackData.sections.clear();
        stagingPlaybackData.events.clear();
        
        int ticksPerQuarterNote = 960;
        int durationTicks = 4 * ticksPerQuarterNote;
        
        PlaybackSection pSec;
        pSec.name = "Preview";
        pSec.startTick = 0;
        pSec.durationTicks = durationTicks;
        pSec.bpm = tempoLocal;
        pSec.loopCount = 1;
        pSec.startSample = 0.0;
        
        for (int ch = 0; ch < 16; ++ch)
        {
            pSec.programs[ch] = 0;
            pSec.volumes[ch] = 0.6f;
        }
        
        int melodicCount = 0;
        for (int i = 0; i < (int) lanes.size(); ++i)
        {
            int channel = 0;
            if (lanes[i].isDrums)
            {
                channel = 9;
            }
            else
            {
                if (melodicCount == 9) melodicCount++;
                channel = melodicCount++;
                if (channel > 15) channel = 15;
            }
            pSec.programs[channel] = lanes[i].gmProgramNumber;
            pSec.volumes[channel] = lanes[i].volume;
        }
        
        double secSamplesPerTick = sampleRateLocal / (16.0 * tempoLocal);
        pSec.durationSamples = durationTicks * secSamplesPerTick;
        
        stagingPlaybackData.sections.push_back (pSec);
        
        juce::Array<int> chordMidiNotes = resolveChordMidiNotes (cb);
        
        melodicCount = 0;
        for (int laneIdx = 0; laneIdx < (int) lanes.size(); ++laneIdx)
        {
            auto& lane = lanes[laneIdx];

            // Determine channel before skipping
            int channel = 0;
            if (lane.isDrums)
            {
                channel = 9;
            }
            else
            {
                if (melodicCount == 9) melodicCount++;
                channel = melodicCount++;
                if (channel > 15) channel = 15;
            }

            if (! lane.enabled || lane.patternId.isEmpty())
                continue;
                
            auto* pattern = PatternDatabase::getInstance().getPatternById (lane.patternId.toStdString());
            if (pattern == nullptr)
                continue;
                
            // Grid-aligned pattern lengths    
            int patternLength = 3840;
            int maxNoteEnd = 0;
            for (auto& n : pattern->notes)
                maxNoteEnd = std::max (maxNoteEnd, n.tick + n.duration);
                
            if (maxNoteEnd > 0)
            {
                patternLength = 960 * ((maxNoteEnd + 959) / 960);
            }
                
            for (int offset = 0; offset < durationTicks; offset += patternLength)
            {
                for (auto& note : pattern->notes)
                {
                    int noteStartInBlock = offset + note.tick;
                    if (noteStartInBlock >= durationTicks)
                        continue;
                        
                    int noteEndInBlock = std::min (durationTicks, noteStartInBlock + note.duration);
                    int absStartTick = noteStartInBlock;
                    int absEndTick = noteEndInBlock;
                    
                    int midiNote = 60;
                    if (note.drumMidi.has_value())
                    {
                        midiNote = note.drumMidi.value();
                    }
                    else if (note.degree.has_value())
                    {
                        if (! chordMidiNotes.isEmpty())
                        {
                            int degree = note.degree.value();
                            int chordSize = chordMidiNotes.size();
                            int wrappedDegree = ((degree % chordSize) + chordSize) % chordSize;
                            int octaveShift = (int)std::floor((float)degree / chordSize);
                            midiNote = chordMidiNotes[wrappedDegree] + (octaveShift * 12);
                            
                            if (lane.gmProgramNumber >= 32 && lane.gmProgramNumber <= 39)
                            {
                                midiNote -= 24;
                            }
                        }
                    }
                    
                    if (! note.drumMidi.has_value())
                    {
                        while (midiNote < 28)  { midiNote += 12; }
                        while (midiNote > 108) { midiNote -= 12; }
                    }
                    
                    SequencerEvent evOn;
                    evOn.tick = absStartTick;
                    evOn.midiNote = midiNote;
                    evOn.velocity = note.velocity;
                    evOn.channel = channel;
                    evOn.isNoteOn = true;
                    stagingPlaybackData.events.push_back (evOn);
                    
                    SequencerEvent evOff;
                    evOff.tick = absEndTick;
                    evOff.midiNote = midiNote;
                    evOff.velocity = 0;
                    evOff.channel = channel;
                    evOff.isNoteOn = false;
                    stagingPlaybackData.events.push_back (evOff);
                }
            }
        }
        
        stagingPlaybackData.totalDurationSamples = pSec.durationSamples;
        
        std::sort (stagingPlaybackData.events.begin(), stagingPlaybackData.events.end(), [](const SequencerEvent& a, const SequencerEvent& b) {
            if (a.tick == b.tick)
                return ! a.isNoteOn && b.isNoteOn;
            return a.tick < b.tick;
        });
    }
    
    PlaybackInstruction swapInst;
    swapInst.type = PlaybackInstruction::SwapProgression;
    queueInstruction (swapInst);
    
    setPlayState (true);
}