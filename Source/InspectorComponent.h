#pragma once
#include <JuceHeader.h>
#include "ChordArrangement.h"

class InspectorComponent  : public juce::Component,
                            public juce::ChangeListener 
{
public:
    InspectorComponent (ChordArrangement& sharedArrangement);
    ~InspectorComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void setChordDetails (const juce::String& name, float totalBeats);
    
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    ChordArrangement& arrangement;

    juce::String currentChordName = "C Maj";
    float totalTimelineBeats = 16.0f;
    
    // Updated: Now applies to the entire chord entry grid
    int gridScrollOffset = 0;

    void drawHardwareButton (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text, juce::Colour baseColour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InspectorComponent)
};