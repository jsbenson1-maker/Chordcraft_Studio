#pragma once

#include <JuceHeader.h>
#include "ChordArrangement.h"

class GlobalMenuComponent : public juce::Component
{
public:
    GlobalMenuComponent (ChordArrangement& arr);
    ~GlobalMenuComponent() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    bool keyPressed (const juce::KeyPress& key) override;
    void visibilityChanged() override;

    std::function<void()> onCloseClicked;
    std::function<void()> onHelpClicked;

private:
    ChordArrangement& arrangement;

    float startDragX = 0;
    float startDragY = 0;
    bool hasSwipedDismiss = false;

    // Sub-view states
    bool isTransposeViewActive = false;
    bool isSettingsViewActive = false;

    // Kept as member variable to prevent premature deletion in async callback
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Dimensions
    int headerHeight = 55;
    int footerHeight = 55;

    // Main grid button areas
    struct GridBounds
    {
        juce::Rectangle<int> leftCol[4];
        juce::Rectangle<int> rightCol[4];
    };
    GridBounds mainGrid;

    // Footer button areas
    juce::Rectangle<int> settingsBtnBounds;
    juce::Rectangle<int> closeBtnBounds;

    // Transpose grid areas
    juce::Rectangle<int> transposeBackBtnBounds;
    juce::Rectangle<int> transposeLeft[6];
    juce::Rectangle<int> transposeRight[6];

    // Settings areas
    juce::Rectangle<int> settingsBackBtnBounds;
    juce::Rectangle<int> loopToggleBounds;
    juce::Rectangle<int> previewToggleBounds;

    void drawHardwareButton (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text, juce::Colour baseColour);
    void transposeArrangement (int semitones);
    void performNew();
    void performSave();
    void performOpen();
    void performExport();
    void performAutoComposer();
    void performHelp();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlobalMenuComponent)
};
