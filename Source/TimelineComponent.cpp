#include "TimelineComponent.h"
#include "ThemeManager.h"

//==============================================================================
TimelineComponent::TimelineComponent(ChordArrangement& sharedArrangement)
    : arrangement(sharedArrangement)
{
    arrangement.addChangeListener(this);
}

TimelineComponent::~TimelineComponent()
{
    arrangement.removeChangeListener(this);
    stopTimer();
}

void TimelineComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

//==============================================================================
void TimelineComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111827)); 

    auto& chords = arrangement.getChords();

    for (int i = 0; i < chords.size(); ++i)
    {
        auto& chord = chords.getReference(i);
        auto bounds = chord.bounds.toFloat();
        float cornerRadius = 6.0f; 

        if (chord.isSelected)
        {
            g.setColour (ThemeManager::getSystemAccentColor().withAlpha(0.2f)); 
            g.fillRoundedRectangle (bounds, cornerRadius);
            g.setColour (ThemeManager::getSystemAccentDark()); 
            g.drawRoundedRectangle (bounds, cornerRadius, 2.0f);
        }
        else
        {
            g.setColour (juce::Colour (0xff1f2937));
            g.fillRoundedRectangle (bounds, cornerRadius);
            g.setColour (juce::Colour (0xff374151)); 
            g.drawRoundedRectangle (bounds, cornerRadius, 1.5f);
        }

        // --- STACKED TEXT LAYOUT ---
        juce::Rectangle<float> contentBounds = bounds.reduced(4.0f);
        
        // Top 55% for Chord Name
        juce::Rectangle<float> textBounds = contentBounds.removeFromTop(contentBounds.getHeight() * 0.55f);
        g.setColour (chord.isSelected ? juce::Colours::white : juce::Colour(0xffd1d5db)); 
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText (chord.name, textBounds, juce::Justification::centredBottom, false);

        // Bottom remaining space for Time Signature
        juce::Rectangle<float> measureBounds = contentBounds;
        g.setColour (chord.isSelected ? ThemeManager::getSystemAccentColor() : juce::Colour(0xff6b7280)); 
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("4/4", measureBounds, juce::Justification::centredTop, false);
    }
}

void TimelineComponent::resized()
{
    auto area = getLocalBounds().reduced (12);
    auto& chords = arrangement.getChords();
    
    int columns = 4; 
    int spacing = 10;
    int blockWidth = (area.getWidth() - (spacing * (columns - 1))) / columns;
    int blockHeight = 60; 

    for (int i = 0; i < chords.size(); ++i)
    {
        int row = i / columns;
        int col = i % columns;
        int x = area.getX() + col * (blockWidth + spacing);
        int y = area.getY() + row * (blockHeight + spacing);
        chords.getReference(i).bounds = juce::Rectangle<int>(x, y, blockWidth, blockHeight);
    }
}

//==============================================================================
int TimelineComponent::getChordIndexAtPosition (juce::Point<int> position)
{
    auto& chords = arrangement.getChords();
    for (int i = 0; i < chords.size(); ++i)
        if (chords[i].bounds.contains (position)) return i;
    return -1;
}

void TimelineComponent::mouseDown (const juce::MouseEvent& event)
{
    int clickedIndex = getChordIndexAtPosition (event.getPosition());
    if (clickedIndex != -1)
    {
        lastMouseDownPosition = event.getPosition();
        dragStartIndex = clickedIndex;

        if (arrangement.isClipboardModeActive) {
            updateDragSelection(clickedIndex);
            return;
        }
        
        startTimer(400); 
    }
}

void TimelineComponent::timerCallback()
{
    stopTimer();
    arrangement.isClipboardModeActive = true;
    auto& chords = arrangement.getChords();
    chords.getReference(dragStartIndex).isSelected = true;
    arrangement.notifyChanges();
}

void TimelineComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (event.getDistanceFromDragStart() > 10 && isTimerRunning()) {
        stopTimer();
    }

    if (arrangement.isClipboardModeActive) {
        int currentIndex = getChordIndexAtPosition (event.getPosition());
        
        if (currentIndex != -1 && currentIndex != lastHapticIndex)
        {
            lastHapticIndex = currentIndex;
            
            #if JUCE_ANDROID
                // TODO: Inject Android JNI Vibrator/HapticGenerator call here
            #endif
        }

        updateDragSelection (currentIndex);
    }
}

void TimelineComponent::mouseUp (const juce::MouseEvent& event)
{
    if (isTimerRunning())
    {
        stopTimer();
        arrangement.isClipboardModeActive = false; 
        
        auto& chords = arrangement.getChords();
        for (auto& c : chords) c.isSelected = false;
        
        if (dragStartIndex != -1) {
            chords.getReference(dragStartIndex).isSelected = true;
            
            #if JUCE_ANDROID
                // TODO: Inject Android JNI Haptic Impact Light call here
            #endif
        }
        arrangement.notifyChanges();
    }
    
    lastHapticIndex = -1; 
}

void TimelineComponent::updateDragSelection (int currentIndex)
{
    if (dragStartIndex == -1 || currentIndex == -1) return;

    int first = juce::jmin (dragStartIndex, currentIndex);
    int last  = juce::jmax (dragStartIndex, currentIndex);
    
    auto& chords = arrangement.getChords();
    for (int i = 0; i < chords.size(); ++i)
        chords.getReference(i).isSelected = (i >= first && i <= last);
        
    arrangement.notifyChanges();
}