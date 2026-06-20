#pragma once
#include <JuceHeader.h>

struct ChordBlock
{
    juce::String name;
    int durationBeats = 4; 
    juce::Colour colour;
    bool isSelected = false;
    juce::Rectangle<int> bounds; 
};

class ChordArrangement : public juce::ChangeBroadcaster
{
public:
    ChordArrangement()
    {
        // Initial placeholder progression
        chords.add ({ "C Maj",  4, juce::Colour (0xff0f172a), false }); 
        chords.add ({ "G Min",  4, juce::Colour (0xff0f172a), false });
        chords.add ({ "A Min",  4, juce::Colour (0xff0f172a), false });
        chords.add ({ "F Maj7", 4, juce::Colour (0xff0f172a), false });
    }

    // NEW: Global state so the Inspector knows exactly when to show the Clipboard Drawer
    bool isClipboardModeActive = false; 

    juce::Array<ChordBlock>& getChords() { return chords; }

    void notifyChanges()
    {
        sendChangeMessage(); // Notifies the UI to repaint
    }

private:
    juce::Array<ChordBlock> chords;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordArrangement)
};