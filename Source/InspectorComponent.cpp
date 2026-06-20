#include "InspectorComponent.h"

//==============================================================================
InspectorComponent::InspectorComponent(ChordArrangement& sharedArrangement)
    : arrangement(sharedArrangement)
{
    arrangement.addChangeListener(this);
}

InspectorComponent::~InspectorComponent()
{
    arrangement.removeChangeListener(this);
}

void InspectorComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

//==============================================================================
void InspectorComponent::drawHardwareButton (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text, juce::Colour baseColour)
{
    auto pad = bounds.reduced(3).toFloat();
    float cornerRadius = 6.0f;
    
    g.setColour (baseColour.withAlpha(0.2f));
    g.fillRoundedRectangle (pad, cornerRadius);
    g.setColour (baseColour.withAlpha(0.9f)); 
    g.drawRoundedRectangle (pad, cornerRadius, 1.5f);
    
    g.setColour (juce::Colours::white); 
    g.setFont (juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText (text, pad, juce::Justification::centred, false);
}

//==============================================================================
void InspectorComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e2e)); 
    auto area = getLocalBounds().reduced (12);

    // =========================================================================
    // GLOBAL TRANSPORT BAR (Play / Stop) - Pinned to bottom
    // =========================================================================
    auto playBar = area.removeFromBottom(45);
    
    // Explicit UTF-8 bytes to fix the crash on the Play and Stop symbols
    drawHardwareButton(g, playBar, juce::String::fromUTF8("\xe2\x96\xb6  PLAY   |   \xe2\x96\xa0  STOP"), juce::Colour(0xff3b82f6));
    area.removeFromBottom(10); // Spacer above play bar

    // =========================================================================
    // MODE 1: SELECTION & CLIPBOARD VIEW
    // =========================================================================
    int selectedCount = 0;
    for (auto& c : arrangement.getChords()) if (c.isSelected) selectedCount++;

    if (arrangement.isClipboardModeActive) 
    {
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
        g.drawText (juce::String(selectedCount) + " Chords Selected", area.removeFromTop(40), juce::Justification::centred, false);
        
        auto actionRow1 = area.removeFromTop(60).reduced(10, 0);
        auto actionRow2 = area.removeFromTop(60).reduced(10, 0);
        
        drawHardwareButton(g, actionRow1.removeFromLeft(actionRow1.getWidth()/2), "Copy", juce::Colour(0xff0ea5e9));
        drawHardwareButton(g, actionRow1, "Delete", juce::Colour(0xffef4444));
        
        int third = actionRow2.getWidth() / 3;
        drawHardwareButton(g, actionRow2.removeFromLeft(third), "Paste (Insert)", juce::Colour(0xff10b981));
        drawHardwareButton(g, actionRow2.removeFromLeft(third), "Paste (Overwrite)", juce::Colour(0xfff59e0b));
        drawHardwareButton(g, actionRow2, "Next Section", juce::Colour(0xff8b5cf6));
        
        auto closeRow = area.removeFromBottom(50);
        drawHardwareButton(g, closeRow, "Close Selection", juce::Colour(0xff64748b));
        return; 
    }

    // =========================================================================
    // MODE 2: HARMONIC ENTRY (Standard View)
    // =========================================================================
    auto topRow1 = area.removeFromTop(45);
    auto topRow2 = area.removeFromTop(45);
    area.removeFromTop(10); 

    juce::Colour btnGrey (0xff475569);
    
    auto accidentals = topRow1.removeFromLeft(150);
    drawHardwareButton(g, accidentals.removeFromLeft(50), juce::String::fromUTF8("\xe2\x99\xad"), btnGrey); 
    drawHardwareButton(g, accidentals.removeFromLeft(50), juce::String::fromUTF8("\xe2\x99\xae"), btnGrey); 
    drawHardwareButton(g, accidentals.removeFromLeft(50), juce::String::fromUTF8("\xe2\x99\xaf"), btnGrey); 
    
    auto actions = topRow1.removeFromRight(200);
    drawHardwareButton(g, actions.removeFromRight(100), "Remove", juce::Colour(0xffef4444)); 
    drawHardwareButton(g, actions.removeFromRight(100), "Insert", juce::Colour(0xff10b981)); 

    drawHardwareButton(g, topRow2.removeFromLeft(120), "Key: C Maj", btnGrey);
    drawHardwareButton(g, topRow2.removeFromLeft(120), "Bass: C", btnGrey);
    drawHardwareButton(g, topRow2.removeFromLeft(140), "Inversions: Root", btnGrey);

    int totalWidth = area.getWidth();
    auto rootCol = area.removeFromLeft(static_cast<int>(totalWidth * 0.15f));
    area.removeFromLeft(8); 
    auto durationCol = area.removeFromRight(static_cast<int>(totalWidth * 0.15f));
    area.removeFromRight(8); 
    
    auto qualitiesArea = area;
    int qWidth = qualitiesArea.getWidth() / 3;
    auto qCol1 = qualitiesArea.removeFromLeft(qWidth); 
    auto qCol2 = qualitiesArea.removeFromLeft(qWidth); 
    auto qCol3 = qualitiesArea;                        

    auto drawGridCol = [&](juce::Rectangle<int> col, juce::StringArray items, juce::Colour baseColour, int scrollOffset = 0) 
    {
        int cellHeight = 45; 
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        
        for (int i = 0; i < items.size(); ++i)
        {
            juce::Rectangle<int> cellBounds (col.getX(), col.getY() + (i * cellHeight) + scrollOffset, col.getWidth(), cellHeight);
            if (cellBounds.getY() > col.getBottom() || cellBounds.getBottom() < col.getY()) continue;
            drawHardwareButton(g, cellBounds, items[i], baseColour);
        }
    };

    juce::StringArray allDurations = {"1/4", "2/4", "3/4", "4/4", "5/4", "6/4", "7/4", "7/8", "9/8", "11/8", "12/8"};

    // Lock the drawing region so scrolling items don't bleed out over the top menus or transport bar
    g.saveState();
    g.reduceClipRegion(area); 

    // Draw all columns with the unified gridScrollOffset
    drawGridCol (rootCol, {"C", "D", "E", "F", "G", "A", "B"}, juce::Colour(0xff94a3b8), gridScrollOffset); 
    drawGridCol (qCol1, {"Maj", "6", "Maj7", "Maj9", "Maj11", "Maj13", "Sus2"}, juce::Colour(0xff10b981), gridScrollOffset); 
    drawGridCol (qCol2, {"Min", "Min6", "Min7", "Min9", "Min11", "Min13", "Sus4"}, juce::Colour(0xfff59e0b), gridScrollOffset); 
    drawGridCol (qCol3, {"Dim", "Aug", "7", "9", "11", "13", "5"}, juce::Colour(0xffef4444), gridScrollOffset); 
    drawGridCol (durationCol, allDurations, juce::Colour(0xff8b5cf6), gridScrollOffset);

    g.restoreState();
}

void InspectorComponent::resized() {}

void InspectorComponent::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    gridScrollOffset += static_cast<int>(wheel.deltaY * 100.0f);
    if (gridScrollOffset > 0) gridScrollOffset = 0;
    
    // Limits the scrolling so you don't scroll infinitely into empty space
    int maxScroll = -250; 
    if (gridScrollOffset < maxScroll) gridScrollOffset = maxScroll;
    repaint();
}

void InspectorComponent::setChordDetails (const juce::String& name, float totalBeats)
{
    currentChordName = name;
    totalTimelineBeats = totalBeats;
    repaint();
}

void InspectorComponent::mouseDown (const juce::MouseEvent& event) {}