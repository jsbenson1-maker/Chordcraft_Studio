#include "ChordcraftAudioProcessor.h"
#include "ChordArrangement.h"
#include "ChordDatabase.h"
#include <cmath>

//==============================================================================
class SineWaveSound : public juce::SynthesiserSound
{
public:
    SineWaveSound() {}
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    SineWaveVoice() {}
    
    bool canPlaySound (juce::SynthesiserSound* sound) override 
    { 
        return dynamic_cast<SineWaveSound*> (sound) != nullptr; 
    }
    
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        currentAngle = 0.0;
        angleDelta = juce::double_Pi * 2.0 * juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber) / getSampleRate();
        level = velocity * 0.12f; // Keep levels safe to prevent clipping
        tailOff = 0.0;
    }
    
    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }
    
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0)
            {
                for (int s = 0; s < numSamples; ++s)
                {
                    auto val = (float) (std::sin (currentAngle) * level * tailOff);
                    for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
                        outputBuffer.addSample (c, startSample + s, val);
                        
                    currentAngle += angleDelta;
                    tailOff *= 0.99; // Simple exponential decay
                    
                    if (tailOff < 0.005)
                    {
                        clearCurrentNote();
                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                for (int s = 0; s < numSamples; ++s)
                {
                    auto val = (float) (std::sin (currentAngle) * level);
                    for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
                        outputBuffer.addSample (c, startSample + s, val);
                        
                    currentAngle += angleDelta;
                }
            }
        }
    }
    
private:
    double currentAngle = 0.0;
    double angleDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;
};

//==============================================================================
ChordcraftAudioProcessor::ChordcraftAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Initialize standard synthesiser voices (polyphony limit = 32 for dense jazz chords & overlapping tails)
    for (int i = 0; i < 32; ++i)
        synth.addVoice (new SineWaveVoice());
        
    synth.addSound (new SineWaveSound());
}

ChordcraftAudioProcessor::~ChordcraftAudioProcessor()
{
}

//==============================================================================
void ChordcraftAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateLocal = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    currentSamplePosition = 0;
    updateSequencerTiming();
}

void ChordcraftAudioProcessor::releaseResources()
{
    synth.allNotesOff (1, true);
}

void ChordcraftAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that are not mapped to inputs
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 1. Dispatch lock-free commands (UI thread -> audio thread)
    processQueueInstructions();

    // 2. Process our playback sequencer timeline triggers
    runSequencer (buffer.getNumSamples(), midiMessages);

    // 3. Render MIDI synth audio into the output buffers
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
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

void ChordcraftAudioProcessor::sendProgressionToAudioThread (const juce::Array<ChordBlock>& chords)
{
    // Clear existing progression on audio thread first
    PlaybackInstruction clearInst;
    clearInst.type = PlaybackInstruction::ClearProgression;
    queueInstruction (clearInst);
    
    int startTick = 0;
    const int ticksPerQuarterNote = 960;
    
    for (int i = 0; i < chords.size(); ++i)
    {
        auto& cb = chords.getReference(i);
        PlaybackInstruction inst;
        inst.type = PlaybackInstruction::UpdateProgressionBlock;
        inst.blockIndex = i;
        inst.startTick = startTick;
        inst.durationTicks = cb.durationBeats * ticksPerQuarterNote;
        
        // Generate O(1) lookup key for database (e.g. C_Maj_i0)
        juce::String id = cb.root + "_" + cb.quality + "_i" + juce::String (cb.inversion);
        juce::zeromem (inst.chordId, sizeof (inst.chordId));
        id.copyToUTF8 (inst.chordId, sizeof (inst.chordId) - 1);
        
        queueInstruction (inst);
        startTick += inst.durationTicks;
    }
}

//==============================================================================
// AUDIO THREAD ONLY CALLS (Lock-free, Allocation-free)
//==============================================================================
void ChordcraftAudioProcessor::processQueueInstructions()
{
    PlaybackInstruction inst;
    while (instructionQueue.pop (inst))
    {
        switch (inst.type)
        {
            case PlaybackInstruction::NoteOn:
                synth.noteOn (1, inst.midiNote, (float) inst.velocity / 127.0f);
                break;
                
            case PlaybackInstruction::NoteOff:
                synth.noteOff (1, inst.midiNote, 0.0f, true);
                break;
                
            case PlaybackInstruction::AllNotesOff:
                synth.allNotesOff (1, true);
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
                    synth.allNotesOff (1, true);
                    for (int i = 0; i < numLocalProgressionBlocks; ++i)
                        localProgression[i].isCurrentlyPlaying = false;
                    currentSamplePosition = 0;
                }
                break;
                
            case PlaybackInstruction::ClearProgression:
                numLocalProgressionBlocks = 0;
                break;
                
            case PlaybackInstruction::UpdateProgressionBlock:
                if (inst.blockIndex >= 0 && inst.blockIndex < maxTimelineBlocks)
                {
                    auto& block = localProgression[inst.blockIndex];
                    block.startTick = inst.startTick;
                    block.durationTicks = inst.durationTicks;
                    
                    // Allocation-free query to ChordDatabase using standard std::string key
                    std::string keyId (inst.chordId);
                    auto* def = ChordDatabase::getInstance().getChordById (keyId);
                    if (def != nullptr)
                    {
                        // Convert root name to MIDI root pitch (C4 Middle C = 60)
                        int rootMidi = 60;
                        if (def->root == "Db") rootMidi = 61;
                        else if (def->root == "D") rootMidi = 62;
                        else if (def->root == "Eb") rootMidi = 63;
                        else if (def->root == "E") rootMidi = 64;
                        else if (def->root == "F") rootMidi = 65;
                        else if (def->root == "Gb") rootMidi = 66;
                        else if (def->root == "G") rootMidi = 67;
                        else if (def->root == "Ab") rootMidi = 68;
                        else if (def->root == "A") rootMidi = 69;
                        else if (def->root == "Bb") rootMidi = 70;
                        else if (def->root == "B") rootMidi = 71;
                        
                        block.numNotes = juce::jmin (8, (int) def->intervals.size());
                        for (int n = 0; n < block.numNotes; ++n)
                        {
                            block.midiNotes[n] = rootMidi + def->intervals[n] + def->rootMidiOffset;
                        }
                    }
                    else
                    {
                        // Default fallback: C Major triad
                        block.numNotes = 3;
                        block.midiNotes[0] = 60;
                        block.midiNotes[1] = 64;
                        block.midiNotes[2] = 67;
                    }
                    block.isCurrentlyPlaying = false;
                    
                    if (inst.blockIndex >= numLocalProgressionBlocks)
                        numLocalProgressionBlocks = inst.blockIndex + 1;
                }
                break;
        }
    }
}

void ChordcraftAudioProcessor::runSequencer (int numSamples, juce::MidiBuffer& midiMessages)
{
    if (! isPlayingLocal || samplesPerTick <= 0.0)
        return;

    double nextSamplePosition = currentSamplePosition + numSamples;
    int startTick = static_cast<int> (currentSamplePosition / samplesPerTick);
    int endTick = static_cast<int> (nextSamplePosition / samplesPerTick);
    
    playheadTickAtomic.set (startTick);

    for (int i = 0; i < numLocalProgressionBlocks; ++i)
    {
        auto& block = localProgression[i];
        
        // Trigger Note On
        if (block.startTick >= startTick && block.startTick < endTick)
        {
            // Calculate sample-accurate offset within this audio block
            int sampleOffset = static_cast<int> ((block.startTick - startTick) * samplesPerTick);
            sampleOffset = juce::jlimit (0, numSamples - 1, sampleOffset);
            
            // Turn off any currently playing block's notes first to prevent overlaps
            for (int prevIdx = 0; prevIdx < numLocalProgressionBlocks; ++prevIdx)
            {
                auto& prevBlock = localProgression[prevIdx];
                if (prevBlock.isCurrentlyPlaying)
                {
                    for (int n = 0; n < prevBlock.numNotes; ++n)
                        midiMessages.addEvent (juce::MidiMessage::noteOff (1, prevBlock.midiNotes[n]), sampleOffset);
                        
                    prevBlock.isCurrentlyPlaying = false;
                }
            }
            
            // Trigger new notes at sampleOffset
            for (int n = 0; n < block.numNotes; ++n)
                midiMessages.addEvent (juce::MidiMessage::noteOn (1, block.midiNotes[n], 0.75f), sampleOffset);
                
            block.isCurrentlyPlaying = true;
        }
        
        // Trigger Note Off
        int blockEndTick = block.startTick + block.durationTicks;
        if (block.isCurrentlyPlaying && blockEndTick >= startTick && blockEndTick < endTick)
        {
            int sampleOffset = static_cast<int> ((blockEndTick - startTick) * samplesPerTick);
            sampleOffset = juce::jlimit (0, numSamples - 1, sampleOffset);
            
            for (int n = 0; n < block.numNotes; ++n)
                midiMessages.addEvent (juce::MidiMessage::noteOff (1, block.midiNotes[n]), sampleOffset);
                
            block.isCurrentlyPlaying = false;
        }
    }

    currentSamplePosition = nextSamplePosition;
    
    // Check if we reached the end of the entire arrangement to loop back
    int totalDurationTicks = 0;
    if (numLocalProgressionBlocks > 0)
    {
        auto& lastBlock = localProgression[numLocalProgressionBlocks - 1];
        totalDurationTicks = lastBlock.startTick + lastBlock.durationTicks;
    }
    
    if (totalDurationTicks > 0)
    {
        double totalDurationSamples = totalDurationTicks * samplesPerTick;
        if (currentSamplePosition >= totalDurationSamples)
        {
            currentSamplePosition = 0;
            for (int i = 0; i < numLocalProgressionBlocks; ++i)
                localProgression[i].isCurrentlyPlaying = false;
                
            // Turn off all notes at loop boundary
            int loopOffset = juce::jlimit (0, numSamples - 1, static_cast<int> (totalDurationSamples - currentSamplePosition));
            midiMessages.addEvent (juce::MidiMessage::allNotesOff (1), loopOffset);
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
