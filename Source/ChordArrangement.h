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
    juce::Colour colour;
    bool isSelected = false;
    juce::Rectangle<int> bounds; 
    juce::Array<int> midiNotes; // Resolved MIDI notes for audio playback
};

class ChordArrangement : public juce::ChangeBroadcaster
{
public:
    ChordArrangement()
    {
        // Initial placeholder progression
        chords.add ({ "C Maj",  "C", "Maj",  0, 4, juce::Colour (0xff0f172a), false, {}, {} }); 
        chords.add ({ "G Min",  "G", "Min",  0, 4, juce::Colour (0xff0f172a), false, {}, {} });
        chords.add ({ "A Min",  "A", "Min",  0, 4, juce::Colour (0xff0f172a), false, {}, {} });
        chords.add ({ "F Maj7", "F", "Maj7", 0, 4, juce::Colour (0xff0f172a), false, {}, {} });
    }

    // Bind the audio processor to enable UI control of the audio thread
    void setAudioProcessor (ChordcraftAudioProcessor* processor)
    {
        audioProcessor = processor;
        if (audioProcessor != nullptr)
            sendProgressionToAudioThread();
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

    void setTempo (float bpm)
    {
        if (audioProcessor != nullptr)
            audioProcessor->setTempo (bpm);
    }

    void sendProgressionToAudioThread()
    {
        if (audioProcessor != nullptr)
            audioProcessor->sendProgressionToAudioThread (chords);
    }

    // NEW: Global state so the Inspector knows exactly when to show the Clipboard Drawer
    bool isClipboardModeActive = false; 

    juce::Array<ChordBlock>& getChords() { return chords; }

    void notifyChanges()
    {
        sendChangeMessage(); // Notifies the UI to repaint
    }

private:
    ChordcraftAudioProcessor* audioProcessor = nullptr;
    juce::Array<ChordBlock> chords;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordArrangement)
};