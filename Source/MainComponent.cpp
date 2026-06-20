#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // 1. Create the Audio/MIDI scheduling engine
    audioProcessor = std::make_unique<ChordcraftAudioProcessor>();

    // 2. Initialize device manager with default inputs (0) and outputs (2)
    deviceManager.initialiseWithDefaultDevices (0, 2);

    // 3. Connect audio processor to the player callback
    processorPlayer.setProcessor (audioProcessor.get());

    // 4. Attach processor player to the audio device callback list
    deviceManager.addAudioCallback (&processorPlayer);

    // 5. Connect UI arrangement to the engine via the lock-free bridge
    arrangement.setAudioProcessor (audioProcessor.get());

    addAndMakeVisible (timeline);
    addAndMakeVisible (inspector);
    
    // Standard mobile portrait resolution for testing
    setSize (430, 932); 
}

MainComponent::~MainComponent()
{
    // Detach audio callbacks first to prevent calling freed memory
    deviceManager.removeAudioCallback (&processorPlayer);
    processorPlayer.setProcessor (nullptr);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffe8e8e8)); // Light background
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Inspector gets the bottom drawer
    inspector.setBounds (area.removeFromBottom (500));
    
    // Timeline gets the top arranger space
    timeline.setBounds (area);
}