#pragma once
#include <JuceHeader.h>
#include "ChordArrangement.h"
#include "TimelineComponent.h"
#include "InspectorComponent.h"
#include "TrackMixerComponent.h"
#include "ChordcraftAudioProcessor.h"
#include "GlobalMenuComponent.h"
#include "SectionTabBarComponent.h"
#include "HelpManualComponent.h"
#include "AppLookAndFeel.h"

class MainComponent  : public juce::Component,
                       private juce::Timer
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
    SectionTabBarComponent sectionTabBar { arrangement };
    juce::Viewport timelineViewport;
    TimelineComponent timeline { arrangement };
    InspectorComponent inspector { arrangement };
    TrackMixerComponent mixer { arrangement };
    GlobalMenuComponent menu { arrangement };
    HelpManualComponent manual;
    AppLookAndFeel lookAndFeel;

    bool isMixerVisible = false;
    bool isMenuVisible = false;
    bool isManualVisible = false;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};