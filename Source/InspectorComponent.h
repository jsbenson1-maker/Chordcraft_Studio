#pragma once
#include <JuceHeader.h>
#include "ChordArrangement.h"
#include "AiCoreManager.h"


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
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;
    void mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

private:
    ChordArrangement& arrangement;

    juce::String currentChordName = "C Maj";
    float totalTimelineBeats = 16.0f;
    
    // Updated: Now applies to the entire chord entry grid
    int gridScrollOffset = 0;

    // UI layout bounds for the bottom transport/control bar
    juce::Rectangle<int> playButtonBounds;
    juce::Rectangle<int> exportBounds;
    juce::Rectangle<int> bpmBounds;

    // UI layout bounds for the top menu bar
    juce::Rectangle<int> keyButtonBounds;
    juce::Rectangle<int> bassButtonBounds;
    juce::Rectangle<int> invButtonBounds;
    juce::Rectangle<int> insertButtonBounds;
    juce::Rectangle<int> removeButtonBounds;

    // Accidentals buttons bounds
    juce::Rectangle<int> flatButtonBounds;
    juce::Rectangle<int> naturalButtonBounds;
    juce::Rectangle<int> sharpButtonBounds;

    // Chord Category Selector bounds and state
    juce::Rectangle<int> categoryButtonBounds;
    juce::String currentCategory = "Common Chords";


    float initialDragBpm = 120.0f;
    
    // Background render thread for offline WAV exporting
    std::unique_ptr<class OfflineRenderThread> exportThread;

    // AI Core advice integration
    bool isAdviceModeActive = false;
    std::unique_ptr<class AiCoreManager> aiManager;
    struct HarmonicAdvice currentAdvice;
    juce::StringArray lastHistory; // Cache of history to prevent duplicate tasks

    juce::Array<ChordBlock> clipboard;

    void updateAiAdvice();
    void drawHardwareButton (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text, juce::Colour baseColour);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InspectorComponent)
};

