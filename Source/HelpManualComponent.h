#pragma once

#include <JuceHeader.h>
#include "HelpManualPages.h"
#include "ThemeManager.h"

class HelpManualComponent : public juce::Component
{
public:
    HelpManualComponent()
    {
        setWantsKeyboardFocus (true);

        addAndMakeVisible (textEditor);
        textEditor.setMultiLine (true);
        textEditor.setReadOnly (true);
        textEditor.setCaretVisible (false);
        textEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff111827));
        textEditor.setColour (juce::TextEditor::textColourId, juce::Colour (0xfff3f4f6));
        textEditor.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        textEditor.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));

        addAndMakeVisible (prevButton);
        addAndMakeVisible (nextButton);
        addAndMakeVisible (closeButton);
        addAndMakeVisible (pageLabel);

        prevButton.setButtonText ("Previous");
        nextButton.setButtonText ("Next");
        closeButton.setButtonText ("Close");

        pageLabel.setJustificationType (juce::Justification::centred);
        pageLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9ca3af));

        prevButton.onClick = [this] { changePage (-1); };
        nextButton.onClick = [this] { changePage (1); };
        closeButton.onClick = [this] {
            ThemeManager::triggerHapticImpact();
            if (onCloseClicked != nullptr)
                onCloseClicked();
        };

        updatePage();
    }

    ~HelpManualComponent() override = default;

    std::function<void()> onCloseClicked;

    void paint (juce::Graphics& g) override
    {
        // Dark premium background with rounded corners and drop shadow
        g.setColour (juce::Colour (0xff1e1e2e));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

        g.setColour (ThemeManager::getSystemAccentColor());
        g.drawRoundedRectangle (getLocalBounds().toFloat(), 8.0f, 2.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (16);
        
        // Safe-area constraints on mobile (status bar/notch/home indicator)
        #if JUCE_ANDROID || JUCE_IOS
        if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        {
            auto insets = display->safeAreaInsets;
            area.removeFromTop (insets.getTop());
            area.removeFromBottom (insets.getBottom());
        }
        #endif

        // Top row
        auto topRow = area.removeFromTop (40);
        closeButton.setBounds (topRow.removeFromRight (80).reduced (2));
        pageLabel.setBounds (topRow);

        // Bottom row
        auto bottomRow = area.removeFromBottom (40);
        prevButton.setBounds (bottomRow.removeFromLeft (100).reduced (2));
        nextButton.setBounds (bottomRow.removeFromRight (100).reduced (2));

        // Middle area for TextEditor
        area.removeFromBottom (10); // spacing
        textEditor.setBounds (area);
    }

    void visibilityChanged() override
    {
        if (isVisible())
            grabKeyboardFocus();
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key.getKeyCode() == juce::KeyPress::escapeKey)
        {
            ThemeManager::triggerHapticImpact();
            if (onCloseClicked != nullptr)
                onCloseClicked();
            return true;
        }
        return false;
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        startX = event.position.getX();
        startY = event.position.getY();
        hasSwiped = false;
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        if (hasSwiped) return;

        float deltaX = event.position.getX() - startX;
        float deltaY = event.position.getY() - startY;

        if (std::abs (deltaX) > 60.0f && std::abs (deltaY) < 30.0f)
        {
            hasSwiped = true;
            if (startX < 40.0f && deltaX > 80.0f) // iOS left-edge swipe to dismiss
            {
                ThemeManager::triggerHapticImpact();
                if (onCloseClicked != nullptr)
                    onCloseClicked();
            }
            else
            {
                // Page turn swipe
                if (deltaX < -60.0f) // Swipe left -> Next page
                {
                    if (currentPage < HelpManualPages::numPages - 1)
                    {
                        ThemeManager::triggerHapticRatchet();
                        changePage (1);
                    }
                }
                else if (deltaX > 60.0f) // Swipe right -> Prev page
                {
                    if (currentPage > 0)
                    {
                        ThemeManager::triggerHapticRatchet();
                        changePage (-1);
                    }
                }
            }
        }
    }

private:
    juce::TextEditor textEditor;
    juce::TextButton prevButton;
    juce::TextButton nextButton;
    juce::TextButton closeButton;
    juce::Label pageLabel;
    int currentPage = 0;

    float startX = 0;
    float startY = 0;
    bool hasSwiped = false;

    void changePage (int delta)
    {
        currentPage = juce::jlimit (0, HelpManualPages::numPages - 1, currentPage + delta);
        updatePage();
    }

    void updatePage()
    {
        textEditor.setText (HelpManualPages::pages[currentPage]);
        pageLabel.setText ("Page " + juce::String (currentPage + 1) + " of " + juce::String (HelpManualPages::numPages), juce::dontSendNotification);
        prevButton.setEnabled (currentPage > 0);
        nextButton.setEnabled (currentPage < HelpManualPages::numPages - 1);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelpManualComponent)
};
