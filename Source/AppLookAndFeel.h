#pragma once
#include <JuceHeader.h>

class AppLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AppLookAndFeel()
    {
        // Style alert colors
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff111827));
        setColour (juce::AlertWindow::textColourId, juce::Colour (0xfff3f4f6));
        setColour (juce::AlertWindow::outlineColourId, juce::Colour (0xff374151));
        
        // Buttons
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1f2937));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3b82f6));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xfff3f4f6));
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);

        // TextEditor inside alerts (e.g. BPM input)
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff1f2937));
        setColour (juce::TextEditor::textColourId, juce::Colour (0xfff3f4f6));
        setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff374151));
        setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xff3b82f6));
    }

    void drawAlertBox (juce::Graphics& g, juce::AlertWindow& alert,
                       const juce::Rectangle<int>& textArea, juce::TextLayout& textLayout) override
    {
        auto bounds = alert.getLocalBounds().toFloat();
        float cornerRadius = 12.0f;

        // 1. Draw elegant dark background
        g.setColour (juce::Colour (0xff111827));
        g.fillRoundedRectangle (bounds, cornerRadius);

        // 2. Draw border
        juce::Colour borderColour = juce::Colour (0xff374151); // default gray
        if (alert.getAlertType() == juce::MessageBoxIconType::WarningIcon)
            borderColour = juce::Colour (0xffef4444); // red
        else if (alert.getAlertType() == juce::MessageBoxIconType::InfoIcon)
            borderColour = juce::Colour (0xff3b82f6); // blue
        else if (alert.getAlertType() == juce::MessageBoxIconType::QuestionIcon)
            borderColour = juce::Colour (0xff8b5cf6); // purple

        g.setColour (borderColour.withAlpha (0.6f));
        g.drawRoundedRectangle (bounds, cornerRadius, 2.0f);

        // 3. Suppress the drawing of the application icon within dialog bounds


        // 4. Draw the text layout
        textLayout.draw (g, textArea.toFloat());
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        float cornerRadius = 6.0f;

        if (shouldDrawButtonAsDown)
            g.setColour (juce::Colour (0xff1d4ed8)); // Darker blue
        else if (shouldDrawButtonAsHighlighted)
            g.setColour (juce::Colour (0xff3b82f6)); // Active accent blue
        else
            g.setColour (juce::Colour (0xff1f2937)); // Default gray-button

        g.fillRoundedRectangle (bounds, cornerRadius);

        // Subtle border
        g.setColour (juce::Colour (0xff374151));
        g.drawRoundedRectangle (bounds, cornerRadius, 1.0f);
    }
};
