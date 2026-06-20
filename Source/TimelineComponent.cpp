#include "TimelineComponent.h"
#include "ThemeManager.h"

//==============================================================================
TimelineComponent::TimelineComponent(ChordArrangement& sharedArrangement)
    : arrangement(sharedArrangement)
{
    arrangement.addChangeListener(this);
    startTimer (30);
}

TimelineComponent::~TimelineComponent()
{
    arrangement.removeChangeListener(this);
    stopTimer();
}

void TimelineComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    resized();
    repaint();
}

//==============================================================================
juce::String TimelineComponent::getInversionText (int inversion, int octave)
{
    juce::String octStr;
    if (octave == 0) octStr = "Low";
    else if (octave == 2) octStr = "High";
    else octStr = "Mid";

    juce::String invStr;
    if (inversion == 1) invStr = "first";
    else if (inversion == 2) invStr = "second";
    else invStr = "root";

    return octStr + " " + invStr;
}

//==============================================================================
void TimelineComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111827)); 

    auto& chords = arrangement.getChords();
    bool isPlaying = arrangement.isPlaying();
    int playheadTick = arrangement.getPlayheadTick();

    int currentStartTick = 0;
    const int ticksPerQuarterNote = 960;

    for (int i = 0; i < chords.size(); ++i)
    {
        auto& chord = chords.getReference(i);
        auto bounds = chord.bounds.toFloat();
        float cornerRadius = 6.0f; 

        int blockDurationTicks = chord.durationBeats * ticksPerQuarterNote;
        int blockEndTick = currentStartTick + blockDurationTicks;

        // Base block colors
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

        // --- PLAYBACK WIPE OVERLAY ---
        if (isPlaying && playheadTick >= currentStartTick && playheadTick < blockEndTick)
        {
            double progress = (double) (playheadTick - currentStartTick) / blockDurationTicks;
            float progressF = (float) juce::jlimit (0.0, 1.0, progress);
            
            // Draw active border or glow
            g.setColour (ThemeManager::getSystemAccentColor().withAlpha(0.5f));
            g.drawRoundedRectangle (bounds, cornerRadius, 2.5f);
            
            // Draw progress wipe
            auto wipeBounds = bounds;
            wipeBounds.setWidth (bounds.getWidth() * progressF);
            
            g.saveState();
            juce::Path clipPath;
            clipPath.addRoundedRectangle (bounds, cornerRadius);
            g.reduceClipRegion (clipPath);
            
            g.setColour (ThemeManager::getSystemAccentColor().withAlpha(0.25f));
            g.fillRect (wipeBounds);
            g.restoreState();
        }

        // --- STACKED TEXT LAYOUT ---
        juce::Rectangle<float> contentBounds = bounds.reduced(4.0f);
        float totalHeight = contentBounds.getHeight();
        
        juce::Rectangle<float> nameBounds = contentBounds.removeFromTop (totalHeight * 0.45f);
        juce::Rectangle<float> sigBounds = contentBounds.removeFromTop (totalHeight * 0.30f);
        juce::Rectangle<float> invBounds = contentBounds;

        // Chord Name
        g.setColour (chord.isSelected ? juce::Colours::white : juce::Colour(0xffd1d5db)); 
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (chord.name, nameBounds, juce::Justification::centredBottom, false);

        // Time Signature
        g.setColour (chord.isSelected ? ThemeManager::getSystemAccentColor() : juce::Colour(0xff9ca3af)); 
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (chord.durationText, sigBounds, juce::Justification::centred, false);

        // Inversion Text
        g.setColour (chord.isSelected ? ThemeManager::getSystemAccentColor().withAlpha(0.8f) : juce::Colour(0xff6b7280)); 
        g.setFont (juce::FontOptions (9.0f));
        g.drawText (getInversionText (chord.inversion, chord.octave), invBounds, juce::Justification::centredTop, false);

        currentStartTick = blockEndTick;
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
        
        mouseDownTime = juce::Time::getMillisecondCounter();
    }
    else
    {
        dragStartIndex = -1;
        if (arrangement.isClipboardModeActive)
        {
            arrangement.isClipboardModeActive = false;
            auto& chords = arrangement.getChords();
            for (auto& c : chords) c.isSelected = false;
            arrangement.notifyChanges();
        }
    }
}

void TimelineComponent::timerCallback()
{
    if (arrangement.isPlaying())
    {
        repaint();
    }

    if (dragStartIndex != -1 && ! arrangement.isClipboardModeActive)
    {
        if (juce::ModifierKeys::getCurrentModifiers().isLeftButtonDown())
        {
            auto now = juce::Time::getMillisecondCounter();
            if (now - mouseDownTime >= 400)
            {
                arrangement.isClipboardModeActive = true;
                auto& chords = arrangement.getChords();
                chords.getReference(dragStartIndex).isSelected = true;
                arrangement.notifyChanges();
            }
        }
    }
}

void TimelineComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (arrangement.isClipboardModeActive) {
        int currentIndex = getChordIndexAtPosition (event.getPosition());
        
        // --- HAPTIC RATCHET TRIGGER ---
        if (currentIndex != -1 && currentIndex != lastHapticIndex)
        {
            lastHapticIndex = currentIndex;
            ThemeManager::triggerHapticRatchet(); 
        }

        updateDragSelection (currentIndex);
    }
}

void TimelineComponent::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    if (! arrangement.isClipboardModeActive)
    {
        auto& chords = arrangement.getChords();
        for (auto& c : chords) c.isSelected = false;
        
        if (dragStartIndex != -1) {
            chords.getReference(dragStartIndex).isSelected = true;
            ThemeManager::triggerHapticImpact(); 
        }
        arrangement.notifyChanges();
    }
    
    dragStartIndex = -1;
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