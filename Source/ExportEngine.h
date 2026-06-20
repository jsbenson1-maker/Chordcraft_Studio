#pragma once

#include <JuceHeader.h>
#include "SynthVoices.h"
#include "ChordDatabase.h"
#include "ChordArrangement.h"
#include <string>

// Helper to get the export directory (Documents folder)
inline juce::File getExportDirectory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
}

// Multitrack MIDI Export (Type 1)
inline void performMidiExport (const juce::Array<ChordBlock>& chords, float bpm)
{
    // Track 0: Meta/Tempo track
    juce::MidiMessageSequence track0;
    track0.addEvent (juce::MidiMessage::tempoMetaEvent (static_cast<int> (60000000.0 / bpm)), 0);
    track0.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    track0.addEvent (juce::MidiMessage::textMetaEvent (3, "Tempo/Meta"), 0);
    
    // Track 1: Bass / root notes
    juce::MidiMessageSequence track1;
    track1.addEvent (juce::MidiMessage::textMetaEvent (3, "Bass"), 0);
    
    // Track 2: Upper chord voicings
    juce::MidiMessageSequence track2;
    track2.addEvent (juce::MidiMessage::textMetaEvent (3, "Chords"), 0);


    
    int currentStartTick = 0;
    const int ticksPerQuarterNote = 960;
    
    for (auto& cb : chords)
    {
        int durationTicks = cb.durationBeats * ticksPerQuarterNote;
        
        // Resolve notes using ChordDatabase
        std::string keyId = (cb.root + "_" + cb.quality + "_i" + juce::String (cb.inversion)).toStdString();
        const auto* def = ChordDatabase::getInstance().getChordById (keyId);
        
        juce::Array<int> notes;
        if (def != nullptr)
        {
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
            
            int numNotes = juce::jmin (8, (int) def->intervals.size());
            for (int n = 0; n < numNotes; ++n)
                notes.add (rootMidi + def->intervals[n] + def->rootMidiOffset);
        }
        else
        {
            // Default fallback
            notes.add (60);
            notes.add (64);
            notes.add (67);
        }
        
        int startTick = currentStartTick;
        int endTick = currentStartTick + durationTicks;
        
        if (notes.size() > 0)
        {
            // Bass / root note goes to Track 1 (MIDI Channel 1)
            track1.addEvent (juce::MidiMessage::noteOn (1, notes[0], 0.75f), startTick);
            track1.addEvent (juce::MidiMessage::noteOff (1, notes[0]), endTick);
            
            // Upper chord voicings go to Track 2 (MIDI Channel 1)
            for (int n = 1; n < notes.size(); ++n)
            {
                track2.addEvent (juce::MidiMessage::noteOn (1, notes[n], 0.75f), startTick);
                track2.addEvent (juce::MidiMessage::noteOff (1, notes[n]), endTick);
            }
        }
        
        currentStartTick += durationTicks;
    }
    
    track0.updateMatchedPairs();
    track1.updateMatchedPairs();
    track2.updateMatchedPairs();
    
    juce::MidiFile midiFile;
    midiFile.setTicksPerQuarterNote (ticksPerQuarterNote);
    midiFile.addTrack (track0);
    midiFile.addTrack (track1);
    midiFile.addTrack (track2);
    
    juce::File midFile = getExportDirectory().getChildFile ("Chordcraft_Studio_Export.mid");
    if (midFile.existsAsFile())
        midFile.deleteFile();
        
    std::unique_ptr<juce::FileOutputStream> stream = midFile.createOutputStream();
    if (stream != nullptr && stream->openedOk())
    {
        midiFile.writeTo (*stream);
        stream.reset(); // close stream cleanly
        
        juce::MessageManager::callAsync ([midFile]() {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                "MIDI Export Successful",
                "Multitrack MIDI file saved to Documents:\n" + midFile.getFullPathName(),
                "OK"
            );
        });
    }
    else
    {
        juce::MessageManager::callAsync ([]() {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "MIDI Export Failed",
                "Could not create output stream for the MIDI file.",
                "OK"
            );
        });
    }
}

// Background thread for Offline Audio Rendering (WAV)
class OfflineRenderThread : public juce::Thread
{
public:
    OfflineRenderThread (const juce::Array<ChordBlock>& chordsToRender, float bpmValue)
        : juce::Thread ("Offline Render Thread"),
          chords (chordsToRender),
          bpm (bpmValue)
    {
    }
    
    ~OfflineRenderThread() override
    {
        stopThread (4000);
    }
    
    void run() override
    {
        // 1. Setup output file
        juce::File docsDir = getExportDirectory();
        juce::File wavFile = docsDir.getChildFile ("Chordcraft_Studio_Export.wav");
        if (wavFile.existsAsFile())
            wavFile.deleteFile();
            
        // 2. Setup internal synthesiser
        juce::Synthesiser synth;
        for (int i = 0; i < 32; ++i)
            synth.addVoice (new SineWaveVoice());
        synth.addSound (new SineWaveSound());
        
        double sampleRate = 44100.0;
        synth.setCurrentPlaybackSampleRate (sampleRate);
        
        // 3. Construct a complete MIDI buffer mapping chords to sample indexes
        juce::MidiBuffer midiBuffer;
        double samplesPerTick = sampleRate / (16.0 * bpm);
        int currentStartTick = 0;
        const int ticksPerQuarterNote = 960;
        
        for (auto& cb : chords)
        {
            int durationTicks = cb.durationBeats * ticksPerQuarterNote;
            
            // Resolve notes using ChordDatabase
            std::string keyId = (cb.root + "_" + cb.quality + "_i" + juce::String (cb.inversion)).toStdString();
            const auto* def = ChordDatabase::getInstance().getChordById (keyId);
            
            juce::Array<int> notes;
            if (def != nullptr)
            {
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
                
                int numNotes = juce::jmin (8, (int) def->intervals.size());
                for (int n = 0; n < numNotes; ++n)
                    notes.add (rootMidi + def->intervals[n] + def->rootMidiOffset);
            }
            else
            {
                notes.add (60);
                notes.add (64);
                notes.add (67);
            }
            
            int startSample = static_cast<int> (currentStartTick * samplesPerTick);
            int endSample = static_cast<int> ((currentStartTick + durationTicks) * samplesPerTick);
            
            for (int note : notes)
            {
                midiBuffer.addEvent (juce::MidiMessage::noteOn (1, note, 0.75f), startSample);
                midiBuffer.addEvent (juce::MidiMessage::noteOff (1, note), endSample);
            }
            
            currentStartTick += durationTicks;
        }
        
        int totalSamples = static_cast<int> (currentStartTick * samplesPerTick);
        
        // 4. Setup Wav audio writer
        juce::WavAudioFormat wavFormat;
        auto* rawStream = wavFile.createOutputStream().release();
        if (rawStream == nullptr)
        {
            showNotification ("WAV Export Failed", "Could not create output stream for the WAV file.");
            return;
        }
        
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wavFormat.createWriterFor (rawStream, sampleRate, 2, 24, {}, 0)
        );
        
        if (writer == nullptr)
        {
            // rawStream is deleted inside createWriterFor if it returns nullptr
            showNotification ("WAV Export Failed", "Could not instantiate the WAV audio format writer.");
            return;
        }
        
        // 5. Offline Render Loop (faster than real-time, no real-time block interaction)
        const int blockSize = 512;
        juce::AudioBuffer<float> renderBuffer (2, blockSize);
        int samplesRendered = 0;
        
        while (samplesRendered < totalSamples && ! threadShouldExit())
        {
            int numSamplesThisBlock = juce::jmin (blockSize, totalSamples - samplesRendered);
            
            // Extract the MIDI events corresponding to this block offset
            juce::MidiBuffer blockMidi;
            blockMidi.addEvents (midiBuffer, samplesRendered, numSamplesThisBlock, -samplesRendered);
            
            renderBuffer.setSize (2, numSamplesThisBlock, false, false, true);
            renderBuffer.clear();
            
            synth.renderNextBlock (renderBuffer, blockMidi, 0, numSamplesThisBlock);
            
            writer->writeFromAudioSampleBuffer (renderBuffer, 0, numSamplesThisBlock);
            samplesRendered += numSamplesThisBlock;
        }
        
        writer.reset(); // Safely closes and flushes the wave file to disk
        
        if (threadShouldExit())
        {
            wavFile.deleteFile();
            showNotification ("WAV Export Cancelled", "WAV render operation was cancelled by user.");
        }
        else
        {
            showNotification ("WAV Export Successful", "Rendered 24-bit WAV file saved to Documents:\n" + wavFile.getFullPathName());
        }
    }
    
private:
    juce::Array<ChordBlock> chords;
    float bpm;
    
    void showNotification (const juce::String& title, const juce::String& message)
    {
        juce::MessageManager::callAsync ([title, message]() {
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                title,
                message,
                "OK"
            );
        });
    }
};
