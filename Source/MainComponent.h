#pragma once
#include <JuceHeader.h>
#include "ChordArrangement.h"
#include "TimelineComponent.h"
#include "InspectorComponent.h"
#include "ChordcraftAudioProcessor.h"

class MainComponent  : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Audio hardware routing & engine
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorPlayer processorPlayer;
    std::unique_ptr<ChordcraftAudioProcessor> audioProcessor;

    // Central data model
    ChordArrangement arrangement;
    
    // UI Components that listen to the data
    TimelineComponent timeline { arrangement };
    InspectorComponent inspector { arrangement };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};