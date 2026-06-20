#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible (timeline);
    addAndMakeVisible (inspector);
    
    // Standard mobile portrait resolution for testing
    setSize (430, 932); 
}

MainComponent::~MainComponent()
{
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