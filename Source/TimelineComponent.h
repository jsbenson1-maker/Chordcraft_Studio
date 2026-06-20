#pragma once
#include <JuceHeader.h>
#include "ChordArrangement.h"

class TimelineComponent  : public juce::Component,
                           public juce::ChangeListener,
                           public juce::Timer 
{
public:
    TimelineComponent (ChordArrangement& sharedArrangement);
    ~TimelineComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    
    void timerCallback() override;

private:
    ChordArrangement& arrangement;

    bool isSelectionModeActive = false;
    int dragStartIndex = -1;
    juce::Point<int> lastMouseDownPosition;
    
    int lastHapticIndex = -1;

    juce::uint32 mouseDownTime = 0;
    juce::String getInversionText (int inversion, int octave);

    int getChordIndexAtPosition (juce::Point<int> position);
    void updateDragSelection (int currentIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimelineComponent)
};