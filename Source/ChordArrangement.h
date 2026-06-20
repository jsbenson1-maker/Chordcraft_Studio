#pragma once
#include <JuceHeader.h>
#include "ChordcraftAudioProcessor.h"

struct ChordBlock
{
    juce::String name;
    juce::String root;
    juce::String quality;
    int inversion = 0;
    int durationBeats = 4; 
    juce::String durationText = "4/4";
    juce::Colour colour;
    bool isSelected = false;
    juce::Rectangle<int> bounds; 
    juce::Array<int> midiNotes; // Resolved MIDI notes for audio playback
    int octave = 1; // 0 = Low, 1 = Mid, 2 = High (default 1)
};

class ChordArrangement : public juce::ChangeBroadcaster
{
public:
    ChordArrangement()
    {
        // Initial placeholder progression
        chords.add ({ "C Maj",  "C", "Maj",  0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 }); 
        chords.add ({ "G Min",  "G", "Min",  0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
        chords.add ({ "A Min",  "A", "Min",  0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
        chords.add ({ "F Maj7", "F", "Maj7", 0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
    }

    // Bind the audio processor to enable UI control of the audio thread
    void setAudioProcessor (ChordcraftAudioProcessor* processor)
    {
        audioProcessor = processor;
        if (audioProcessor != nullptr)
        {
            sendProgressionToAudioThread();
            setTempo (bpm);
        }
    }

    int getPlayheadTick() const
    {
        if (audioProcessor != nullptr)
            return audioProcessor->getPlayheadTick();
        return 0;
    }

    void auditionNoteOn (int midiNote, int velocity)
    {
        if (audioProcessor != nullptr)
            audioProcessor->auditionNoteOn (midiNote, velocity);
    }

    void auditionNoteOff (int midiNote)
    {
        if (audioProcessor != nullptr)
            audioProcessor->auditionNoteOff (midiNote);
    }

    void clearAudition()
    {
        if (audioProcessor != nullptr)
            audioProcessor->clearAudition();
    }

    void setPlayState (bool play)
    {
        if (audioProcessor != nullptr)
            audioProcessor->setPlayState (play);
    }

    bool isPlaying() const
    {
        if (audioProcessor != nullptr)
            return audioProcessor->isEnginePlaying();
        return false;
    }

    void setTempo (float newBpm)
    {
        bpm = newBpm;
        if (audioProcessor != nullptr)
            audioProcessor->setTempo (newBpm);
        notifyChanges();
    }

    float getBpm() const { return bpm; }

    void sendProgressionToAudioThread()
    {
        if (audioProcessor != nullptr)
            audioProcessor->sendProgressionToAudioThread (chords);
    }

    // NEW: Global state so the Inspector knows exactly when to show the Clipboard Drawer
    bool isClipboardModeActive = false; 

    juce::String activeKey = "C Maj";

    juce::Array<ChordBlock>& getChords() { return chords; }

    void notifyChanges()
    {
        sendChangeMessage(); // Notifies the UI to repaint
    }

private:
    ChordcraftAudioProcessor* audioProcessor = nullptr;
    juce::Array<ChordBlock> chords;
    float bpm = 120.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordArrangement)
};