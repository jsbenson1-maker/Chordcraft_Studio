#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    // Apply LookAndFeel globally
    juce::Desktop::getInstance().setDefaultLookAndFeel (&lookAndFeel);

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

    addAndMakeVisible (sectionTabBar);
    addAndMakeVisible (timelineViewport);
    timelineViewport.setViewedComponent (&timeline, false);
    timelineViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (inspector);
    addChildComponent (mixer);
    addChildComponent (menu);
    addChildComponent (manual);

    // Bind callbacks
    inspector.onMixerButtonToggled = [this]()
    {
        isMixerVisible = ! isMixerVisible;
        mixer.setVisible (isMixerVisible);
        if (isMixerVisible)
        {
            mixer.toFront (true);
            isMenuVisible = false;
            menu.setVisible (false);
            isManualVisible = false;
            manual.setVisible (false);
        }
    };

    mixer.onCloseClicked = [this]()
    {
        isMixerVisible = false;
        mixer.setVisible (false);
    };

    inspector.onMenuButtonToggled = [this]()
    {
        isMenuVisible = ! isMenuVisible;
        menu.setVisible (isMenuVisible);
        if (isMenuVisible)
        {
            menu.toFront (true);
            isMixerVisible = false;
            mixer.setVisible (false);
            isManualVisible = false;
            manual.setVisible (false);
        }
    };

    menu.onCloseClicked = [this]()
    {
        isMenuVisible = false;
        menu.setVisible (false);
    };

    menu.onHelpClicked = [this]()
    {
        isManualVisible = true;
        manual.setVisible (true);
        manual.toFront (true);
        isMenuVisible = false;
        menu.setVisible (false);
    };

    manual.onCloseClicked = [this]()
    {
        isManualVisible = false;
        manual.setVisible (false);
    };
    
    // Standard mobile portrait resolution for testing
    setSize (430, 932); 

    // Start MIDI Output scanner (every 2 seconds)
    audioProcessor->updateActiveMidiOutputs();
    startTimer (2000);
}

MainComponent::~MainComponent()
{
    juce::Desktop::getInstance().setDefaultLookAndFeel (nullptr);
    
    // Detach audio callbacks first to prevent calling freed memory
    deviceManager.removeAudioCallback (&processorPlayer);
    processorPlayer.setProcessor (nullptr);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111827)); // Dark background matching the app theme
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    #if JUCE_ANDROID || JUCE_IOS
    int topInset = 48; // fallback for status bar/notch
    int bottomInset = 15; // fallback for home indicator
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        auto insets = display->safeAreaInsets;
        topInset = std::max (topInset, insets.getTop());
        bottomInset = std::max (bottomInset, insets.getBottom());
    }
    area.removeFromTop (topInset);
    area.removeFromBottom (bottomInset);
    #endif
    
    auto overlayArea = area;
    
    // Inspector gets the bottom drawer
    inspector.setBounds (area.removeFromBottom (500));
    
    // Section Tab Bar gets the top 40 pixels
    sectionTabBar.setBounds (area.removeFromTop (40));
    
    // Timeline viewport gets the remaining arranger space
    timelineViewport.setBounds (area);
    
    // Resize timeline component dynamically based on current chords
    int numChords = arrangement.getChords().size();
    int numRows = (numChords + 3) / 4;
    int timelineHeight = std::max (area.getHeight(), numRows * 70 + 24);
    timeline.setSize (timelineViewport.getWidth() - 16, timelineHeight);

    // Overlays cover the entire safe-area bounds
    mixer.setBounds (overlayArea);
    menu.setBounds (overlayArea);
    manual.setBounds (overlayArea);
}

void MainComponent::timerCallback()
{
    audioProcessor->updateActiveMidiOutputs();
}