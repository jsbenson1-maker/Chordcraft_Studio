#include "InspectorComponent.h"
#include "ExportEngine.h"
#include "AiCoreManager.h"
#include "ThemeManager.h"
#include "ChordDatabase.h"

//==============================================================================
namespace
{
    juce::String getBaseRootName (const juce::String& dbRoot)
    {
        if (dbRoot == "C") return "C";
        if (dbRoot == "Db") return "D"; 
        if (dbRoot == "D") return "D";
        if (dbRoot == "Eb") return "E"; 
        if (dbRoot == "E") return "E";
        if (dbRoot == "F") return "F";
        if (dbRoot == "Gb") return "G"; 
        if (dbRoot == "G") return "G";
        if (dbRoot == "Ab") return "A"; 
        if (dbRoot == "A") return "A";
        if (dbRoot == "Bb") return "B"; 
        if (dbRoot == "B") return "B";
        return "C";
    }

    int getAccidentalIndex (const juce::String& dbRoot)
    {
        if (dbRoot == "Db" || dbRoot == "Eb" || dbRoot == "Gb" || dbRoot == "Ab" || dbRoot == "Bb")
            return 0; // Flat
        return 1; // Natural
    }

    juce::String combineRootAndAccidental (const juce::String& base, int acc)
    {
        int basePitch = 0;
        if (base == "C") basePitch = 0;
        else if (base == "D") basePitch = 2;
        else if (base == "E") basePitch = 4;
        else if (base == "F") basePitch = 5;
        else if (base == "G") basePitch = 7;
        else if (base == "A") basePitch = 9;
        else if (base == "B") basePitch = 11;

        int offset = 0;
        if (acc == 0) offset = -1; // Flat
        else if (acc == 2) offset = 1; // Sharp

        int targetPitch = (basePitch + offset + 12) % 12;

        juce::StringArray pitches = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        return pitches[targetPitch];
    }

    juce::StringArray getQualitiesForCategory (const juce::String& category)
    {
        juce::StringArray result;
        auto& allQualities = ChordDatabase::getInstance().getQualities();
        
        if (category == "Common Chords")
        {
            juce::StringArray common = {
                "Maj", "Min", "7", "Maj7", "Min7", "Dim", "Dim7", "m7b5", "Aug", "Aug7",
                "6", "Min6", "6/9", "Maj9", "Min9", "9", "Maj11", "Min11", "11", "Maj13",
                "Min13", "13", "Sus2", "Sus4", "7sus4", "5", "Add9", "MinAdd9", "7#9", "7b9",
                "7#5", "7b5", "9#11", "9b5", "13#11", "13b9", "Maj7#11", "Maj7#5", "MinMaj7", "Min6/9",
                "7#9#5", "7#9b5", "7b9#5", "7b9b5", "11b9", "13#9", "13b5", "13#9b5", "9#5", "9#5#11",
                "7sus4b9", "7sus4#9", "6sus4", "Maj7sus4", "Min7sus4", "7#11", "13sus4", "Sus4b9", "Min11#13", "9sus4"
            };
            
            juce::StringArray available;
            for (auto& c : common)
            {
                if (allQualities.contains (c))
                    available.add (c);
            }
            if (available.size() > 0)
                return available;
                
            return {"Maj", "Min", "Dim", "Aug", "6", "Min6", "Maj7", "Min7", "7", "Maj9", "Min9", "9", "Sus2", "Sus4", "5"};
        }
        
        for (auto& q : allQualities)
        {
            if (category == "Major Extensions")
            {
                if (q.startsWith ("Maj") || q.contains ("add") || q == "6" || q == "6/9" || q.contains ("Maj7") || q.contains ("Maj9"))
                {
                    if (q != "Maj" && q != "6" && q != "Maj7" && q != "Maj9")
                        result.addIfNotAlreadyThere (q);
                }
            }
            else if (category == "Minor Extensions")
            {
                if (q.startsWith ("Min") || q.startsWith ("m") || q.contains ("min"))
                {
                    if (q != "Min" && q != "Min6" && q != "Min7" && q != "Min9" && !q.contains ("m7b5") && !q.contains ("dim"))
                        result.addIfNotAlreadyThere (q);
                }
            }
            else if (category == "Dominant & Altered")
            {
                if (q.contains ("7") || q.contains ("9") || q.contains ("11") || q.contains ("13") || q.contains ("alt"))
                {
                    if (!q.startsWith ("Maj") && !q.startsWith ("Min") && !q.startsWith ("m") && q != "7" && q != "9" && q != "11" && q != "13")
                        result.addIfNotAlreadyThere (q);
                }
            }
            else if (category == "Diminished & Augmented")
            {
                if (q.contains ("Dim") || q.contains ("dim") || q.contains ("Aug") || q.contains ("aug") || q.contains ("o") || q.contains ("ø") || q.contains ("b5"))
                {
                    if (q != "Dim" && q != "Aug")
                        result.addIfNotAlreadyThere (q);
                }
            }
            else if (category == "Suspended & Power")
            {
                if (q.contains ("Sus") || q.contains ("sus") || q == "5")
                {
                    if (q != "Sus2" && q != "Sus4" && q != "5")
                        result.addIfNotAlreadyThere (q);
                }
            }
        }
        
        if (result.isEmpty())
            return allQualities;
            
        return result;
    }

    juce::Colour getQualityColour (const juce::String& root, const juce::String& quality, bool isAdviceActive, const HarmonicAdvice& advice)
    {
        if (isAdviceActive)
        {
            juce::String fullName = root + " " + quality;
            
            for (auto& s : advice.safe)
                if (s.equalsIgnoreCase (fullName)) return juce::Colour (0xff10b981);
                
            for (auto& t : advice.tension)
                if (t.equalsIgnoreCase (fullName)) return juce::Colour (0xfff59e0b);
                
            for (auto& e : advice.experimental)
                if (e.equalsIgnoreCase (fullName)) return juce::Colour (0xffef4444);
        }
        return juce::Colours::transparentBlack;
    }

    juce::String getChordBassNoteName (const juce::String& root, const juce::String& quality, int inversion)
    {
        auto* def = ChordDatabase::getInstance().getChord (root, quality, inversion);
        if (def == nullptr || def->intervals.empty())
            return root;
            
        int rootPitch = 0;
        if (root == "C") rootPitch = 0;
        else if (root == "Db") rootPitch = 1;
        else if (root == "D") rootPitch = 2;
        else if (root == "Eb") rootPitch = 3;
        else if (root == "E") rootPitch = 4;
        else if (root == "F") rootPitch = 5;
        else if (root == "Gb") rootPitch = 6;
        else if (root == "G") rootPitch = 7;
        else if (root == "Ab") rootPitch = 8;
        else if (root == "A") rootPitch = 9;
        else if (root == "Bb") rootPitch = 10;
        else if (root == "B") rootPitch = 11;
        
        int lowestInterval = def->intervals[0];
        for (int interval : def->intervals)
        {
            if (interval < lowestInterval)
                lowestInterval = interval;
        }
        
        int bassPitch = (rootPitch + lowestInterval + def->rootMidiOffset + 120) % 12;
        juce::StringArray pitches = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        return pitches[bassPitch];
    }

    void trySetBassNote (ChordBlock* selectedChord, const juce::String& targetBass)
    {
        if (selectedChord == nullptr) return;
        
        int targetInv = -1;
        for (int inv = 0; inv < 4; ++inv)
        {
            if (getChordBassNoteName (selectedChord->root, selectedChord->quality, inv) == targetBass)
            {
                targetInv = inv;
                break;
            }
        }
        
        if (targetInv != -1)
        {
            selectedChord->inversion = targetInv;
            selectedChord->customBassNote = "";
            auto* def = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, targetInv);
            if (def != nullptr)
                selectedChord->name = def->name;
            else
                selectedChord->name = selectedChord->root + " " + selectedChord->quality;
        }
        else
        {
            selectedChord->customBassNote = targetBass;
            selectedChord->inversion = 0;
            selectedChord->name = selectedChord->root + " " + selectedChord->quality + "/" + targetBass;
        }
    }

    juce::Array<int> getScalePitchClasses (const juce::String& activeKey)
    {
        juce::String keyRoot = "C";
        juce::String keyMode = "Maj";
        
        juce::StringArray keyTokens;
        keyTokens.addTokens (activeKey, " ", "");
        if (keyTokens.size() > 0)
            keyRoot = keyTokens[0];
        if (keyTokens.size() > 1)
            keyMode = keyTokens[1];
            
        auto getPitchClass = [](const juce::String& r) -> int {
            if (r == "C") return 0;
            if (r == "Db" || r == "C#") return 1;
            if (r == "D") return 2;
            if (r == "Eb" || r == "D#") return 3;
            if (r == "E") return 4;
            if (r == "F") return 5;
            if (r == "Gb" || r == "F#") return 6;
            if (r == "G") return 7;
            if (r == "Ab" || r == "G#") return 8;
            if (r == "A") return 9;
            if (r == "Bb" || r == "A#") return 10;
            if (r == "B") return 11;
            return 0;
        };
        
        int rootPitch = getPitchClass (keyRoot);
        juce::Array<int> intervals;
        
        if (keyMode.equalsIgnoreCase ("Maj") || keyMode.equalsIgnoreCase ("Ionian"))
            intervals = { 0, 2, 4, 5, 7, 9, 11 };
        else if (keyMode.equalsIgnoreCase ("Min") || keyMode.equalsIgnoreCase ("Aeolian"))
            intervals = { 0, 2, 3, 5, 7, 8, 10 };
        else if (keyMode.equalsIgnoreCase ("Dorian"))
            intervals = { 0, 2, 3, 5, 7, 9, 10 };
        else if (keyMode.equalsIgnoreCase ("Phrygian"))
            intervals = { 0, 1, 3, 5, 7, 8, 10 };
        else if (keyMode.equalsIgnoreCase ("Lydian"))
            intervals = { 0, 2, 4, 6, 7, 9, 11 };
        else if (keyMode.equalsIgnoreCase ("Mixolydian"))
            intervals = { 0, 2, 4, 5, 7, 9, 10 };
        else if (keyMode.equalsIgnoreCase ("Locrian"))
            intervals = { 0, 1, 3, 5, 6, 8, 10 };
        else
            intervals = { 0, 2, 4, 5, 7, 9, 11 }; // Fallback to Major
            
        juce::Array<int> scale;
        for (int interval : intervals)
            scale.add ((rootPitch + interval) % 12);
            
        return scale;
    }

    juce::Array<int> getChordPitchClasses (const juce::String& root, const juce::String& quality)
    {
        juce::Array<int> chordPitches;
        auto* def = ChordDatabase::getInstance().getChord (root, quality, 0);
        if (def != nullptr)
        {
            auto getPitchClass = [](const juce::String& r) -> int {
                if (r == "C") return 0;
                if (r == "Db" || r == "C#") return 1;
                if (r == "D") return 2;
                if (r == "Eb" || r == "D#") return 3;
                if (r == "E") return 4;
                if (r == "F") return 5;
                if (r == "Gb" || r == "F#") return 6;
                if (r == "G") return 7;
                if (r == "Ab" || r == "G#") return 8;
                if (r == "A") return 9;
                if (r == "Bb" || r == "A#") return 10;
                if (r == "B") return 11;
                return 0;
            };
            int rootPitch = getPitchClass (root);
            for (int interval : def->intervals)
            {
                chordPitches.add ((rootPitch + interval + def->rootMidiOffset + 120) % 12);
            }
        }
        return chordPitches;
    }

    juce::Colour getChordTheoryColour (const juce::String& activeKey, const juce::String& root, const juce::String& quality, juce::Colour fallbackColour = juce::Colour (0xffef4444))
    {
        juce::Array<int> scalePitches = getScalePitchClasses (activeKey);
        juce::Array<int> chordPitches = getChordPitchClasses (root, quality);
        
        if (chordPitches.isEmpty())
            return fallbackColour;
            
        bool allDiatonic = true;
        for (int p : chordPitches)
        {
            if (! scalePitches.contains (p))
            {
                allDiatonic = false;
                break;
            }
        }
        
        if (allDiatonic)
            return juce::Colour (0xff10b981); // Diatonic (Green)
            
        auto getPitchClass = [](const juce::String& r) -> int {
            if (r == "C") return 0;
            if (r == "Db" || r == "C#") return 1;
            if (r == "D") return 2;
            if (r == "Eb" || r == "D#") return 3;
            if (r == "E") return 4;
            if (r == "F") return 5;
            if (r == "Gb" || r == "F#") return 6;
            if (r == "G") return 7;
            if (r == "Ab" || r == "G#") return 8;
            if (r == "A") return 9;
            if (r == "Bb" || r == "A#") return 10;
            if (r == "B") return 11;
            return 0;
        };
        int rootPitch = getPitchClass (root);
        
        if (scalePitches.contains (rootPitch))
            return juce::Colour (0xfff59e0b); // Modal Mixture (Yellow)
            
        juce::String parallelKey = activeKey;
        if (activeKey.containsIgnoreCase ("Maj"))
            parallelKey = activeKey.replace ("Maj", "Min");
        else if (activeKey.containsIgnoreCase ("Min"))
            parallelKey = activeKey.replace ("Min", "Maj");
            
        juce::Array<int> parallelPitches = getScalePitchClasses (parallelKey);
        if (parallelPitches.contains (rootPitch))
            return juce::Colour (0xfff59e0b); // Parallel borrow (Yellow)
            
        return fallbackColour;
    }

    int getPitchClass (const juce::String& r)
    {
        if (r == "C") return 0;
        if (r == "Db" || r == "C#") return 1;
        if (r == "D") return 2;
        if (r == "Eb" || r == "D#") return 3;
        if (r == "E") return 4;
        if (r == "F") return 5;
        if (r == "Gb" || r == "F#") return 6;
        if (r == "G") return 7;
        if (r == "Ab" || r == "G#") return 8;
        if (r == "A") return 9;
        if (r == "Bb" || r == "A#") return 10;
        if (r == "B") return 11;
        return 0;
    }

    juce::String getRootNameFromPitchClass (int pc)
    {
        juce::StringArray pitches = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        return pitches[((pc % 12) + 12) % 12];
    }

    juce::String getDurationTextForBeats (int beats)
    {
        if (beats == 1) return "1/4";
        if (beats == 2) return "2/4";
        if (beats == 3) return "3/4";
        if (beats == 4) return "4/4";
        if (beats == 5) return "5/4";
        if (beats == 6) return "6/4";
        if (beats == 7) return "7/4";
        return "4/4";
    }

    juce::String getCanonicalChordName (const juce::String& root, const juce::String& quality, int inversion)
    {
        auto* def = ChordDatabase::getInstance().getChord (root, quality, inversion);
        if (def != nullptr)
            return def->name;
        return root + " " + quality;
    }
}

//==============================================================================
InspectorComponent::InspectorComponent(ChordArrangement& sharedArrangement)
    : arrangement(sharedArrangement)
{
    arrangement.addChangeListener(this);
    aiManager = std::make_unique<AiCoreManager>();
    updateAiAdvice();
}

InspectorComponent::~InspectorComponent()
{
    arrangement.removeChangeListener(this);
    if (exportThread != nullptr)
    {
        exportThread->signalThreadShouldExit();
        exportThread->stopThread (4000);
        exportThread.reset();
    }
    if (aiManager != nullptr)
    {
        aiManager->signalThreadShouldExit();
        aiManager->stopThread (4000);
        aiManager.reset();
    }
}

void InspectorComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateAiAdvice();
    repaint();
}

void InspectorComponent::updateAiAdvice()
{
    juce::StringArray history;
    for (auto& cb : arrangement.getChords())
    {
        if (cb.name.isNotEmpty())
            history.add (cb.name);
    }
    
    if (history == lastHistory)
        return;
        
    lastHistory = history;
    
    if (aiManager != nullptr)
    {
        aiManager->triggerInferenceAsync (history, arrangement.activeKey, [this](const HarmonicAdvice& advice)
        {
            currentAdvice = advice;
            repaint();
        });
    }
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
    // GLOBAL TRANSPORT BAR, EXPORT & BPM - Pinned to bottom
    // =========================================================================
    auto playBar = area.removeFromBottom(45);
    auto playWidth = playBar.getWidth();
    playButtonBounds = playBar.removeFromLeft(static_cast<int>(playWidth * 0.35f));
    menuBounds = playBar.removeFromLeft(static_cast<int>(playWidth * 0.20f));
    mixerBounds = playBar.removeFromLeft(static_cast<int>(playWidth * 0.20f));
    bpmBounds = playBar;
    
    juce::String playText = arrangement.isPlaying() ? 
        juce::String::fromUTF8("\xe2\x96\xaa  STOP") : 
        juce::String::fromUTF8("\xe2\x96\xb6  PLAY");
    
    drawHardwareButton(g, playButtonBounds, playText, juce::Colour(0xff3b82f6));
    drawHardwareButton(g, menuBounds, "MENU", juce::Colour(0xff8b5cf6));
    drawHardwareButton(g, mixerBounds, "MIXER", ThemeManager::getSystemAccentColor());
    
    juce::String bpmText = "BPM: " + juce::String(arrangement.getBpm(), 1);
    drawHardwareButton(g, bpmBounds, bpmText, juce::Colour(0xff10b981));
    
    area.removeFromBottom(10); // Spacer above play bar

    if (isInversionDrawerActive)
    {
        // Draw inversion drawer layout
        auto titleArea = area.removeFromTop (40);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText ("CHORD INVERSION & OCTAVE", titleArea, juce::Justification::centred, false);
        
        auto gridArea = area.removeFromTop (180);
        int cellW = gridArea.getWidth() / 3;
        int cellH = gridArea.getHeight() / 3;
        
        // Find selected chord
        auto& chords = arrangement.getChords();
        ChordBlock* selectedChord = nullptr;
        for (auto& cb : chords)
        {
            if (cb.isSelected) { selectedChord = &cb; break; }
        }
        
        juce::Colour btnGrey (0xff475569);
        
        juce::StringArray octaveLabels = { "Low", "Mid", "High" };
        juce::StringArray invLabels = { "Root", "1st", "2nd" };
        
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                juce::Rectangle<int> cellBounds (gridArea.getX() + c * cellW, gridArea.getY() + r * cellH, cellW, cellH);
                juce::String cellText = octaveLabels[c] + " " + invLabels[r];
                
                bool isHighlighted = (selectedChord != nullptr && selectedChord->octave == c && selectedChord->inversion == r);
                juce::Colour cellColor = isHighlighted ? ThemeManager::getSystemAccentColor() : btnGrey;
                
                drawHardwareButton (g, cellBounds, cellText, cellColor);
                if (isHighlighted)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.8f));
                    g.drawRoundedRectangle (cellBounds.reduced(2).toFloat(), 6.0f, 2.0f);
                }
            }
        }
        
        area.removeFromTop (15);
        auto controlRow = area.removeFromTop (45);
        int btnW = controlRow.getWidth() / 2;
        auto autoSelectBounds = controlRow.removeFromLeft (btnW);
        auto closeBounds = controlRow;
        
        drawHardwareButton (g, autoSelectBounds, "Auto Select", ThemeManager::getSystemAccentColor());
        drawHardwareButton (g, closeBounds, "Close", juce::Colour (0xffef4444));
        
        return;
    }

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
    
    // Find the currently selected chord block
    auto& chords = arrangement.getChords();
    ChordBlock* selectedChord = nullptr;
    for (auto& cb : chords)
    {
        if (cb.isSelected)
        {
            selectedChord = &cb;
            break;
        }
    }
    
    int currentAcc = 1; // Default to Natural
    juce::String currentRoot = "C";
    if (selectedChord != nullptr)
    {
        currentAcc = getAccidentalIndex (selectedChord->root);
        currentRoot = selectedChord->root;
    }
    
    juce::Colour flatBtnColor = (selectedChord != nullptr && currentAcc == 0) ? ThemeManager::getSystemAccentColor() : btnGrey;
    juce::Colour naturalBtnColor = (selectedChord != nullptr && currentAcc == 1) ? ThemeManager::getSystemAccentColor() : btnGrey;
    juce::Colour sharpBtnColor = btnGrey; // Stateless transient sharp button
    
    auto accidentals = topRow1.removeFromLeft(150);
    flatButtonBounds = accidentals.removeFromLeft(50);
    naturalButtonBounds = accidentals.removeFromLeft(50);
    sharpButtonBounds = accidentals.removeFromLeft(50);
    
    drawHardwareButton(g, flatButtonBounds, juce::String::fromUTF8("\xe2\x99\xad"), flatBtnColor); 
    drawHardwareButton(g, naturalButtonBounds, juce::String::fromUTF8("\xe2\x99\xae"), naturalBtnColor); 
    drawHardwareButton(g, sharpButtonBounds, juce::String::fromUTF8("\xe2\x99\xaf"), sharpBtnColor); 
    
    auto actions = topRow1.removeFromRight(200);
    removeButtonBounds = actions.removeFromRight(100);
    insertButtonBounds = actions;
    drawHardwareButton(g, removeButtonBounds, "Remove", juce::Colour(0xffef4444)); 
    drawHardwareButton(g, insertButtonBounds, "Insert", juce::Colour(0xff10b981)); 
 
    auto row2Width = topRow2.getWidth();
    keyButtonBounds = topRow2.removeFromLeft (static_cast<int> (row2Width * 0.25f));
    bassButtonBounds = topRow2.removeFromLeft (static_cast<int> (row2Width * 0.25f));
    categoryButtonBounds = topRow2.removeFromLeft (static_cast<int> (row2Width * 0.30f)); 
    invButtonBounds = topRow2;
    
    juce::String keyText = "Key: " + arrangement.activeKey;
    juce::String bassText = "Bass: ";
    if (selectedChord)
    {
        if (selectedChord->customBassNote.isNotEmpty())
            bassText += selectedChord->customBassNote;
        else
            bassText += getChordBassNoteName (selectedChord->root, selectedChord->quality, selectedChord->inversion);
    }
    else
    {
        bassText += "C";
    }
    juce::Colour invColor = (selectedChord && selectedChord->inversion > 0) ? ThemeManager::getSystemAccentColor() : btnGrey;
    
    drawHardwareButton(g, keyButtonBounds, keyText, btnGrey);
    drawHardwareButton(g, bassButtonBounds, bassText, btnGrey);
    drawHardwareButton(g, categoryButtonBounds, currentCategory, ThemeManager::getSystemAccentColor());
    drawHardwareButton(g, invButtonBounds, "INV", invColor);

    if (selectedChord != nullptr)
    {
        passingChordButtonBounds = area.removeFromTop(45);
        area.removeFromTop(10);
        drawHardwareButton(g, passingChordButtonBounds, "Insert Passing Chord", juce::Colour(0xff3b82f6));
    }
    else
    {
        passingChordButtonBounds = juce::Rectangle<int>();
    }

    auto gridArea = area;
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

    auto drawGridCol = [&](juce::Rectangle<int> col, juce::StringArray items, juce::Colour baseColour, bool isQualitiesCol, int scrollOffset = 0) 
    {
        int cellHeight = 45; 
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        
        for (int i = 0; i < items.size(); ++i)
        {
            juce::Rectangle<int> cellBounds (col.getX(), col.getY() + (i * cellHeight) + scrollOffset, col.getWidth(), cellHeight);
            if (cellBounds.getY() > col.getBottom() || cellBounds.getBottom() < col.getY()) continue;
            
            juce::Colour drawColour = baseColour;
            if (isQualitiesCol)
            {
                juce::String checkRoot = selectedChord ? selectedChord->root : "C";
                juce::String checkQuality = items[i];
                
                if (items[i].contains (" "))
                {
                    juce::StringArray tokens;
                    tokens.addTokens (items[i], " ", "");
                    if (tokens.size() > 0)
                        checkRoot = tokens[0];
                    if (tokens.size() > 1)
                        checkQuality = tokens[1];
                }
                
                drawColour = getChordTheoryColour (arrangement.activeKey, checkRoot, checkQuality, baseColour);
            }
            
            bool isHighlighted = false;
            if (selectedChord != nullptr)
            {
                if (col == rootCol)
                {
                    isHighlighted = (getBaseRootName(selectedChord->root) == items[i]);
                }
                else if (isQualitiesCol)
                {
                    isHighlighted = (selectedChord->quality == items[i]);
                }
                else if (col == durationCol)
                {
                    int beats = 4;
                    juce::String dur = items[i];
                    if (dur == "1/4") beats = 1;
                    else if (dur == "2/4") beats = 2;
                    else if (dur == "3/4") beats = 3;
                    else if (dur == "4/4") beats = 4;
                    else if (dur == "5/4") beats = 5;
                    else if (dur == "6/4") beats = 6;
                    else if (dur == "7/4") beats = 7;
                    else if (dur == "7/8") beats = 2;
                    else if (dur == "9/8") beats = 3;
                    else if (dur == "11/8") beats = 4;
                    else if (dur == "12/8") beats = 4;
                    
                    isHighlighted = (selectedChord->durationBeats == beats);
                }
            }
            
            juce::Colour buttonColor = isHighlighted ? drawColour.brighter(0.4f) : drawColour;
            drawHardwareButton(g, cellBounds, items[i], buttonColor);
            
            if (isHighlighted)
            {
                g.setColour (juce::Colours::white.withAlpha(0.8f));
                g.drawRoundedRectangle(cellBounds.reduced(2).toFloat(), 6.0f, 2.0f);
            }
        }
    };

    juce::StringArray allDurations = {"1/4", "2/4", "3/4", "4/4", "5/4", "6/4", "7/4", "7/8", "9/8", "11/8", "12/8"};

    // Lock the drawing region so scrolling items don't bleed out over the top menus or transport bar
    g.saveState();
    g.reduceClipRegion(gridArea); 

    // Draw roots and durations using decoupled offsets
    drawGridCol (rootCol, {"C", "D", "E", "F", "G", "A", "B"}, juce::Colour(0xff94a3b8), false, rootsScrollOffset); 
    drawGridCol (durationCol, allDurations, juce::Colour(0xff8b5cf6), false, durationsScrollOffset);

    // Dynamic qualities list based on category & advice
    if (isAdviceModeActive && currentCategory == "Common Chords")
    {
        drawGridCol (qCol1, currentAdvice.safe, juce::Colour(0xff10b981), true, qualitiesScrollOffset); 
        drawGridCol (qCol2, currentAdvice.tension, juce::Colour(0xfff59e0b), true, qualitiesScrollOffset); 
        drawGridCol (qCol3, currentAdvice.experimental, juce::Colour(0xffef4444), true, qualitiesScrollOffset); 
    }
    else
    {
        juce::StringArray displayQualities = getQualitiesForCategory(currentCategory);
        int qCount = displayQualities.size();
        int colCount = (qCount + 2) / 3;
        
        juce::StringArray qList1, qList2, qList3;
        for (int i = 0; i < qCount; ++i)
        {
            if (i < colCount) qList1.add(displayQualities[i]);
            else if (i < colCount * 2) qList2.add(displayQualities[i]);
            else qList3.add(displayQualities[i]);
        }
        
        drawGridCol (qCol1, qList1, juce::Colour(0xff10b981), true, qualitiesScrollOffset); 
        drawGridCol (qCol2, qList2, juce::Colour(0xfff59e0b), true, qualitiesScrollOffset); 
        drawGridCol (qCol3, qList3, juce::Colour(0xffef4444), true, qualitiesScrollOffset); 
    }

    g.restoreState();;
}

void InspectorComponent::resized() {}

void InspectorComponent::constrainScrollOffsets()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromBottom (45);
    area.removeFromBottom (10);
    area.removeFromTop (45); // topRow1
    area.removeFromTop (45); // topRow2
    area.removeFromTop (10); // spacer
    
    bool hasSelection = false;
    for (auto& cb : arrangement.getChords())
    {
        if (cb.isSelected) { hasSelection = true; break; }
    }
    if (hasSelection)
    {
        area.removeFromTop (45); // passingChordButtonBounds
        area.removeFromTop (10); // spacer
    }
    
    int cellHeight = 45;
    
    // Constrain Roots
    if (rootsScrollOffset > 0) rootsScrollOffset = 0;
    int totalRootsHeight = 7 * cellHeight;
    int maxRootsScroll = - (totalRootsHeight - area.getHeight() + 120);
    if (maxRootsScroll > 0) maxRootsScroll = 0;
    if (rootsScrollOffset < maxRootsScroll) rootsScrollOffset = maxRootsScroll;
    
    // Constrain Durations
    if (durationsScrollOffset > 0) durationsScrollOffset = 0;
    int totalDurationsHeight = 11 * cellHeight;
    int maxDurationsScroll = - (totalDurationsHeight - area.getHeight() + 120);
    if (maxDurationsScroll > 0) maxDurationsScroll = 0;
    if (durationsScrollOffset < maxDurationsScroll) durationsScrollOffset = maxDurationsScroll;
    
    // Constrain Qualities
    if (qualitiesScrollOffset > 0) qualitiesScrollOffset = 0;
    
    int totalQualitiesHeight = 0;
    if (isAdviceModeActive && currentCategory == "Common Chords")
    {
        int maxItems = juce::jmax (currentAdvice.safe.size(), currentAdvice.tension.size(), currentAdvice.experimental.size());
        totalQualitiesHeight = maxItems * cellHeight;
    }
    else
    {
        juce::StringArray displayQualities = getQualitiesForCategory (currentCategory);
        int qCount = displayQualities.size();
        int colCount = (qCount + 2) / 3;
        totalQualitiesHeight = colCount * cellHeight;
    }
    
    int maxQualitiesScroll = - (totalQualitiesHeight - area.getHeight() + 120);
    if (maxQualitiesScroll > 0) maxQualitiesScroll = 0;
    if (qualitiesScrollOffset < maxQualitiesScroll) qualitiesScrollOffset = maxQualitiesScroll;
}

void InspectorComponent::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (isInversionDrawerActive)
        return; // No scrolling in inversion modal drawer

    auto pos = event.getPosition();
    auto area = getLocalBounds().reduced (12);
    area.removeFromBottom (45);
    area.removeFromBottom (10);
    area.removeFromTop (45); // topRow1
    area.removeFromTop (45); // topRow2
    area.removeFromTop (10); // spacer
    
    bool hasSelection = false;
    for (auto& cb : arrangement.getChords())
    {
        if (cb.isSelected) { hasSelection = true; break; }
    }
    if (hasSelection)
    {
        area.removeFromTop (45); // passingChordButtonBounds
        area.removeFromTop (10); // spacer
    }
    
    int totalWidth = area.getWidth();
    auto rootCol = area.removeFromLeft (static_cast<int> (totalWidth * 0.15f));
    area.removeFromLeft (8);
    auto durationCol = area.removeFromRight (static_cast<int> (totalWidth * 0.15f));
    area.removeFromRight (8);
    auto qualitiesArea = area;
    
    if (rootCol.contains (pos))
    {
        rootsScrollOffset += static_cast<int>(wheel.deltaY * 100.0f);
    }
    else if (durationCol.contains (pos))
    {
        durationsScrollOffset += static_cast<int>(wheel.deltaY * 100.0f);
    }
    else if (qualitiesArea.contains (pos))
    {
        qualitiesScrollOffset += static_cast<int>(wheel.deltaY * 100.0f);
    }
    
    constrainScrollOffsets();
    repaint();
}

void InspectorComponent::setChordDetails (const juce::String& name, float totalBeats)
{
    currentChordName = name;
    totalTimelineBeats = totalBeats;
    repaint();
}

void InspectorComponent::mouseDown (const juce::MouseEvent& event)
{
    auto pos = event.getPosition();
    
    // Reconstruct all bounds dynamically to match paint() layout
    auto area = getLocalBounds().reduced (12);
    
    auto playBar = area.removeFromBottom (45);
    auto playWidth = playBar.getWidth();
    playButtonBounds = playBar.removeFromLeft (static_cast<int> (playWidth * 0.35f));
    menuBounds = playBar.removeFromLeft (static_cast<int> (playWidth * 0.20f));
    mixerBounds = playBar.removeFromLeft (static_cast<int> (playWidth * 0.20f));
    bpmBounds = playBar;
    
    area.removeFromBottom (10);
    
    // Bottom transport / export / mixer clicks check first
    if (playButtonBounds.contains (pos))
    {
        bool currentPlayState = arrangement.isPlaying();
        arrangement.setPlayState (! currentPlayState);
        repaint();
        return;
    }
    else if (bpmBounds.contains (pos))
    {
        initialDragBpm = arrangement.getBpm();
        return;
    }
    else if (mixerBounds.contains (pos))
    {
        ThemeManager::triggerHapticImpact();
        if (onMixerButtonToggled != nullptr)
            onMixerButtonToggled();
        return;
    }
    else if (menuBounds.contains (pos))
    {
        ThemeManager::triggerHapticImpact();
        if (onMenuButtonToggled != nullptr)
            onMenuButtonToggled();
        return;
    }

    if (isInversionDrawerActive)
    {
        // Handle Inversion Drawer internal clicks with correct coordinates
        auto titleArea = area.removeFromTop (40);
        auto gridArea = area.removeFromTop (180);
        area.removeFromTop (15);
        auto controlRow = area.removeFromTop (45);
        
        int cellW = gridArea.getWidth() / 3;
        int cellH = gridArea.getHeight() / 3;
        
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                juce::Rectangle<int> cellBounds (gridArea.getX() + c * cellW, gridArea.getY() + r * cellH, cellW, cellH);
                if (cellBounds.contains (pos))
                {
                    auto& chords = arrangement.getChords();
                    ChordBlock* selectedChord = nullptr;
                    for (auto& cb : chords)
                    {
                        if (cb.isSelected) { selectedChord = &cb; break; }
                    }
                    
                    if (selectedChord != nullptr)
                    {
                        auto proceedInversionChange = [this, selectedChord, r, c]()
                        {
                            selectedChord->octave = c;
                            selectedChord->inversion = r;
                            selectedChord->customBassNote = "";
                            
                            auto* def = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, selectedChord->inversion);
                            if (def != nullptr)
                                selectedChord->name = def->name;
                            else
                                selectedChord->name = selectedChord->root + " " + selectedChord->quality;
                            
                            arrangement.sendProgressionToAudioThread();
                            arrangement.notifyChanges();
                            repaint();
                        };

                        if (selectedChord->customBassNote.isNotEmpty())
                        {
                            juce::AlertWindow::showOkCancelBox (
                                juce::AlertWindow::QuestionIcon,
                                "Clear Custom Bass Note?",
                                "Changing the inversion will clear your custom bass note (" + selectedChord->customBassNote + "). Proceed?",
                                "Yes",
                                "Cancel",
                                nullptr,
                                juce::ModalCallbackFunction::create ([proceedInversionChange](int result)
                                {
                                    if (result != 0)
                                    {
                                        proceedInversionChange();
                                    }
                                })
                            );
                        }
                        else
                        {
                            proceedInversionChange();
                        }
                        return;
                    }
                    repaint();
                    return;
                }
            }
        }
        
        int btnW = controlRow.getWidth() / 2;
        auto autoSelectBounds = controlRow.removeFromLeft (btnW);
        auto closeBounds = controlRow;
        
        if (autoSelectBounds.contains (pos))
        {
            auto& chords = arrangement.getChords();
            ChordBlock* selectedChord = nullptr;
            for (auto& cb : chords)
            {
                if (cb.isSelected) { selectedChord = &cb; break; }
            }
            
            if (selectedChord != nullptr)
            {
                auto proceedReset = [this, selectedChord]()
                {
                    selectedChord->octave = 1;
                    selectedChord->inversion = 0;
                    selectedChord->customBassNote = "";
                    auto* def = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, 0);
                    if (def != nullptr)
                        selectedChord->name = def->name;
                    else
                        selectedChord->name = selectedChord->root + " " + selectedChord->quality;
                        
                    arrangement.sendProgressionToAudioThread();
                    arrangement.notifyChanges();
                    repaint();
                };
                
                if (selectedChord->customBassNote.isNotEmpty())
                {
                    juce::AlertWindow::showOkCancelBox (
                        juce::AlertWindow::QuestionIcon,
                        "Clear Custom Bass Note?",
                        "Resetting will clear your custom bass note (" + selectedChord->customBassNote + "). Proceed?",
                        "Yes",
                        "Cancel",
                        nullptr,
                        juce::ModalCallbackFunction::create ([proceedReset](int result)
                        {
                            if (result != 0)
                            {
                                proceedReset();
                            }
                        })
                    );
                }
                else
                {
                    proceedReset();
                }
            }
            return;
        }
        else if (closeBounds.contains (pos))
        {
            isInversionDrawerActive = false;
            repaint();
            return;
        }
        
        return; // Consume click if inversion overlay is active
    }

    if (arrangement.isClipboardModeActive) 
    {
        // Reconstruct layout identical to paint()
        area = getLocalBounds().reduced (12);
        area.removeFromBottom(45);
        area.removeFromBottom(10);
        
        // Skip title row (40 pixels high)
        area.removeFromTop(40);
        
        auto actionRow1 = area.removeFromTop(60).reduced(10, 0);
        auto actionRow2 = area.removeFromTop(60).reduced(10, 0);
        
        auto copyBounds = actionRow1.removeFromLeft(actionRow1.getWidth() / 2);
        auto deleteBounds = actionRow1;
        
        int third = actionRow2.getWidth() / 3;
        auto pasteInsertBounds = actionRow2.removeFromLeft(third);
        auto pasteOverwriteBounds = actionRow2.removeFromLeft(third);
        auto nextSectionBounds = actionRow2;
        
        auto closeRowBounds = area.removeFromBottom(50);
        
        if (copyBounds.contains(pos))
        {
            clipboard.clear();
            for (auto& cb : arrangement.getChords())
            {
                if (cb.isSelected)
                {
                    ChordBlock copied = cb;
                    copied.isSelected = false;
                    clipboard.add(copied);
                }
            }
        }
        else if (deleteBounds.contains(pos))
        {
            auto& chords = arrangement.getChords();
            for (int i = chords.size() - 1; i >= 0; --i)
            {
                if (chords[i].isSelected)
                    chords.remove(i);
            }
            if (chords.isEmpty())
            {
                chords.add({ "C Maj", "C", "Maj", 0, 4, "4/4", juce::Colour(0xff0f172a), true, {}, {} });
            }
            else
            {
                chords.getReference(0).isSelected = true;
            }
            arrangement.isClipboardModeActive = false;
            arrangement.sendProgressionToAudioThread();
            arrangement.notifyChanges();
            updateAiAdvice();
        }
        else if (pasteInsertBounds.contains(pos))
        {
            if (! clipboard.isEmpty())
            {
                auto& chords = arrangement.getChords();
                int insertIdx = chords.size();
                for (int i = chords.size() - 1; i >= 0; --i)
                {
                    if (chords[i].isSelected)
                    {
                        insertIdx = i + 1;
                        break;
                    }
                }
                
                for (auto& cb : chords)
                    cb.isSelected = false;
                    
                for (int i = 0; i < clipboard.size(); ++i)
                {
                    ChordBlock pasted = clipboard[i];
                    pasted.isSelected = true;
                    chords.insert(insertIdx + i, pasted);
                }
                arrangement.isClipboardModeActive = false;
                arrangement.sendProgressionToAudioThread();
                arrangement.notifyChanges();
                updateAiAdvice();
            }
        }
        else if (pasteOverwriteBounds.contains(pos))
        {
            if (! clipboard.isEmpty())
            {
                auto& chords = arrangement.getChords();
                int firstSelectedIdx = -1;
                int selectedCount = 0;
                for (int i = 0; i < chords.size(); ++i)
                {
                    if (chords[i].isSelected)
                    {
                        if (firstSelectedIdx == -1)
                            firstSelectedIdx = i;
                        selectedCount++;
                    }
                }
                
                if (firstSelectedIdx != -1)
                {
                    for (int i = 0; i < selectedCount; ++i)
                    {
                        if (firstSelectedIdx < chords.size())
                            chords.remove(firstSelectedIdx);
                    }
                    
                    for (int i = 0; i < clipboard.size(); ++i)
                    {
                        ChordBlock pasted = clipboard[i];
                        pasted.isSelected = true;
                        chords.insert(firstSelectedIdx + i, pasted);
                    }
                    
                    arrangement.isClipboardModeActive = false;
                    arrangement.sendProgressionToAudioThread();
                    arrangement.notifyChanges();
                    updateAiAdvice();
                }
            }
        }
        else if (nextSectionBounds.contains(pos))
        {
            auto& chords = arrangement.getChords();
            juce::Array<ChordBlock> sectionToDuplicate;
            for (auto& cb : chords)
            {
                if (cb.isSelected)
                {
                    ChordBlock dup = cb;
                    dup.isSelected = false;
                    sectionToDuplicate.add(dup);
                }
            }
            
            if (! sectionToDuplicate.isEmpty())
            {
                for (auto& cb : chords)
                    cb.isSelected = false;
                    
                for (int i = 0; i < sectionToDuplicate.size(); ++i)
                {
                    ChordBlock pasted = sectionToDuplicate[i];
                    pasted.isSelected = true;
                    chords.add(pasted);
                }
                arrangement.isClipboardModeActive = false;
                arrangement.sendProgressionToAudioThread();
                arrangement.notifyChanges();
                updateAiAdvice();
            }
        }
        else if (closeRowBounds.contains(pos))
        {
            arrangement.isClipboardModeActive = false;
            for (auto& cb : arrangement.getChords())
                cb.isSelected = false;
            arrangement.notifyChanges();
        }
        
        return;
    }

    // Standard view buttons: Accidentals & Actions
    auto topRow1 = area.removeFromTop (45);
    auto topRow2 = area.removeFromTop (45);
    
    // Check if a chord is selected
    auto& chords = arrangement.getChords();
    ChordBlock* selectedChord = nullptr;
    int selectedIndex = -1;
    for (int i = 0; i < chords.size(); ++i)
    {
        if (chords[i].isSelected)
        {
            selectedChord = &chords.getReference(i);
            selectedIndex = i;
            break;
        }
    }
    
    if (selectedChord != nullptr)
    {
        passingChordButtonBounds = area.removeFromTop (45);
        area.removeFromTop (10);
    }
    else
    {
        passingChordButtonBounds = juce::Rectangle<int>();
    }
    
    if (selectedChord != nullptr && passingChordButtonBounds.contains (pos))
    {
        ThemeManager::triggerHapticImpact();
        
        int nextIndex = (selectedIndex + 1) % chords.size();
        auto& targetChord = chords.getReference (nextIndex);
        
        int targetRootPC = getPitchClass (targetChord.root);
        
        int secDomRootPC = (targetRootPC + 7) % 12;
        int tritoneSubRootPC = (targetRootPC + 1) % 12;
        int leadingToneRootPC = (targetRootPC + 11) % 12;
        
        juce::String secDomRoot = getRootNameFromPitchClass (secDomRootPC);
        juce::String tritoneSubRoot = getRootNameFromPitchClass (tritoneSubRootPC);
        juce::String leadingToneRoot = getRootNameFromPitchClass (leadingToneRootPC);
        
        juce::PopupMenu menu;
        menu.addItem (1, secDomRoot + "7 (Secondary Dominant)");
        menu.addItem (2, tritoneSubRoot + "7 (Tritone Sub)");
        menu.addItem (3, leadingToneRoot + "Dim7 (Leading Tone)");
        
        menu.showMenuAsync (juce::PopupMenu::Options(), [this, selectedIndex, secDomRoot, tritoneSubRoot, leadingToneRoot](int result)
        {
            if (result == 0) return;
            
            auto& chords = arrangement.getChords();
            if (selectedIndex < 0 || selectedIndex >= chords.size()) return;
            
            juce::String chosenRoot;
            juce::String chosenQuality;
            if (result == 1)
            {
                chosenRoot = secDomRoot;
                chosenQuality = "7";
            }
            else if (result == 2)
            {
                chosenRoot = tritoneSubRoot;
                chosenQuality = "7";
            }
            else if (result == 3)
            {
                chosenRoot = leadingToneRoot;
                chosenQuality = "Dim7";
            }
            
            auto& cb = chords.getReference (selectedIndex);
            if (cb.durationBeats > 1)
            {
                cb.durationBeats -= 1;
                cb.durationText = getDurationTextForBeats (cb.durationBeats);
                cb.isSelected = false;
                
                ChordBlock newBlock;
                newBlock.root = chosenRoot;
                newBlock.quality = chosenQuality;
                newBlock.name = getCanonicalChordName (chosenRoot, chosenQuality, 0);
                newBlock.inversion = 0;
                newBlock.durationBeats = 1;
                newBlock.durationText = "1/4";
                newBlock.colour = juce::Colour (0xff0f172a);
                newBlock.isSelected = true;
                newBlock.octave = 1;
                
                chords.insert (selectedIndex + 1, newBlock);
            }
            else
            {
                cb.root = chosenRoot;
                cb.quality = chosenQuality;
                cb.name = getCanonicalChordName (chosenRoot, chosenQuality, cb.inversion);
                cb.customBassNote = "";
            }
            
            arrangement.sendProgressionToAudioThread();
            arrangement.notifyChanges();
            updateAiAdvice();
            repaint();
        });
        
        return;
    }
    
    auto accidentals = topRow1.removeFromLeft (150);
    flatButtonBounds = accidentals.removeFromLeft (50);
    naturalButtonBounds = accidentals.removeFromLeft (50);
    sharpButtonBounds = accidentals.removeFromLeft (50);
    
    auto actions = topRow1.removeFromRight (200);
    removeButtonBounds = actions.removeFromRight (100);
    insertButtonBounds = actions;
    
    auto row2Width = topRow2.getWidth();
    keyButtonBounds = topRow2.removeFromLeft (static_cast<int> (row2Width * 0.25f));
    bassButtonBounds = topRow2.removeFromLeft (static_cast<int> (row2Width * 0.25f));
    categoryButtonBounds = topRow2.removeFromLeft (static_cast<int> (row2Width * 0.30f));
    invButtonBounds = topRow2;

    // Accidentals buttons click handling
    if (flatButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        for (auto& cb : chords)
        {
            if (cb.isSelected)
            {
                juce::String base = getBaseRootName (cb.root);
                cb.root = combineRootAndAccidental (base, 0); // Flat
                cb.name = cb.root + " " + cb.quality;
                break;
            }
        }
        arrangement.sendProgressionToAudioThread();
        arrangement.notifyChanges();
        updateAiAdvice();
        return;
    }
    else if (naturalButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        for (auto& cb : chords)
        {
            if (cb.isSelected)
            {
                juce::String base = getBaseRootName (cb.root);
                cb.root = combineRootAndAccidental (base, 1); // Natural
                cb.name = cb.root + " " + cb.quality;
                break;
            }
        }
        arrangement.sendProgressionToAudioThread();
        arrangement.notifyChanges();
        updateAiAdvice();
        return;
    }
    else if (sharpButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        for (auto& cb : chords)
        {
            if (cb.isSelected)
            {
                juce::String base = getBaseRootName (cb.root);
                cb.root = combineRootAndAccidental (base, 2); // Sharp
                cb.name = cb.root + " " + cb.quality;
                break;
            }
        }
        arrangement.sendProgressionToAudioThread();
        arrangement.notifyChanges();
        updateAiAdvice();
        return;
    }
    else if (categoryButtonBounds.contains (pos))
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Common Chords", true, currentCategory == "Common Chords");
        menu.addItem (2, "Major Extensions", true, currentCategory == "Major Extensions");
        menu.addItem (3, "Minor Extensions", true, currentCategory == "Minor Extensions");
        menu.addItem (4, "Dominant & Altered", true, currentCategory == "Dominant & Altered");
        menu.addItem (5, "Diminished & Augmented", true, currentCategory == "Diminished & Augmented");
        menu.addItem (6, "Suspended & Power", true, currentCategory == "Suspended & Power");
        
        menu.showMenuAsync (juce::PopupMenu::Options(), [this](int result)
        {
            if (result == 1) currentCategory = "Common Chords";
            else if (result == 2) currentCategory = "Major Extensions";
            else if (result == 3) currentCategory = "Minor Extensions";
            else if (result == 4) currentCategory = "Dominant & Altered";
            else if (result == 5) currentCategory = "Diminished & Augmented";
            else if (result == 6) currentCategory = "Suspended & Power";
            
            rootsScrollOffset = 0;
            qualitiesScrollOffset = 0;
            durationsScrollOffset = 0;
            repaint();
        });
        return;
    }

    // Top action row clicks handling
    if (insertButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        int insertIdx = chords.size();
        for (int i = 0; i < chords.size(); ++i)
        {
            if (chords[i].isSelected)
            {
                insertIdx = i + 1;
                chords.getReference(i).isSelected = false;
                break;
            }
        }
        
        ChordBlock newChord { "C Maj", "C", "Maj", 0, 4, "4/4", juce::Colour (0xff0f172a), true, {}, {} };
        chords.insert (insertIdx, newChord);
        arrangement.sendProgressionToAudioThread();
        arrangement.notifyChanges();
        updateAiAdvice();
        return;
    }
    else if (removeButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        for (int i = chords.size() - 1; i >= 0; --i)
        {
            if (chords[i].isSelected)
                chords.remove (i);
        }
        
        if (chords.size() == 0)
        {
            chords.add ({ "C Maj", "C", "Maj", 0, 4, "4/4", juce::Colour (0xff0f172a), true, {}, {} });
        }
        else
        {
            chords.getReference (juce::jmin (chords.size() - 1, 0)).isSelected = true;
        }
        
        arrangement.sendProgressionToAudioThread();
        arrangement.notifyChanges();
        updateAiAdvice();
        return;
    }
    else if (keyButtonBounds.contains (pos))
    {
        juce::PopupMenu menu;
        juce::StringArray roots = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        juce::StringArray modes = { "Maj", "Min", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Locrian" };
        
        for (int r = 0; r < roots.size(); ++r)
        {
            juce::PopupMenu modeSubMenu;
            for (int m = 0; m < modes.size(); ++m)
            {
                juce::String keyName = roots[r] + " " + modes[m];
                bool isCurrent = (arrangement.activeKey == keyName);
                modeSubMenu.addItem (r * 100 + m + 1, keyName, true, isCurrent);
            }
            menu.addSubMenu (roots[r], modeSubMenu);
        }
        
        menu.showMenuAsync (juce::PopupMenu::Options(), [this, roots, modes](int result)
        {
            if (result >= 1)
            {
                int rIdx = (result - 1) / 100;
                int mIdx = (result - 1) % 100;
                if (rIdx >= 0 && rIdx < roots.size() && mIdx >= 0 && mIdx < modes.size())
                {
                    arrangement.activeKey = roots[rIdx] + " " + modes[mIdx];
                    arrangement.notifyChanges();
                    updateAiAdvice();
                }
            }
        });
        return;
    }
    else if (bassButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        int selectedIndex = -1;
        for (int i = 0; i < chords.size(); ++i)
        {
            if (chords[i].isSelected) { selectedIndex = i; break; }
        }
        
        if (selectedIndex != -1)
        {
            auto& selectedChord = chords.getReference (selectedIndex);
            juce::PopupMenu menu;
            
            juce::StringArray pitches = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
            juce::Array<int> scalePitches = getScalePitchClasses (arrangement.activeKey);
            juce::Array<int> chordPitches = getChordPitchClasses (selectedChord.root, selectedChord.quality);
            
            juce::String currentBass;
            if (selectedChord.customBassNote.isNotEmpty())
                currentBass = selectedChord.customBassNote;
            else
                currentBass = getChordBassNoteName (selectedChord.root, selectedChord.quality, selectedChord.inversion);
                
            auto getPitchClass = [](const juce::String& r) -> int {
                if (r == "C") return 0;
                if (r == "Db" || r == "C#") return 1;
                if (r == "D") return 2;
                if (r == "Eb" || r == "D#") return 3;
                if (r == "E") return 4;
                if (r == "F") return 5;
                if (r == "Gb" || r == "F#") return 6;
                if (r == "G") return 7;
                if (r == "Ab" || r == "G#") return 8;
                if (r == "A") return 9;
                if (r == "Bb" || r == "A#") return 10;
                if (r == "B") return 11;
                return 0;
            };
            
            for (int i = 0; i < pitches.size(); ++i)
            {
                int pClass = getPitchClass (pitches[i]);
                bool isChordTone = chordPitches.contains (pClass);
                bool isKeyTone = scalePitches.contains (pClass);
                
                juce::String label = pitches[i];
                if (pitches[i] == selectedChord.root)
                    label += " (Chord Root)";
                else if (isChordTone)
                    label += " (Chord Tone)";
                else if (isKeyTone)
                    label += " (Key Tone)";
                    
                bool isSuggested = isChordTone || isKeyTone;
                bool isTicked = (currentBass == pitches[i]);
                
                if (isSuggested)
                    menu.addColouredItem (i + 1, label, juce::Colour (0xff10b981), true, isTicked);
                else
                    menu.addItem (i + 1, label, true, isTicked);
            }
            
            menu.showMenuAsync (juce::PopupMenu::Options(), [this, selectedIndex, pitches](int result)
            {
                if (result >= 1 && result <= pitches.size())
                {
                    auto& chords = arrangement.getChords();
                    if (selectedIndex >= 0 && selectedIndex < chords.size())
                    {
                        auto& selectedChord = chords.getReference (selectedIndex);
                        juce::String targetBass = pitches[result - 1];
                        
                        int targetInv = -1;
                        for (int inv = 0; inv < 4; ++inv)
                        {
                            if (getChordBassNoteName (selectedChord.root, selectedChord.quality, inv) == targetBass)
                            {
                                targetInv = inv;
                                break;
                            }
                        }
                        
                        if (targetInv != -1)
                        {
                            selectedChord.inversion = targetInv;
                            selectedChord.customBassNote = "";
                            
                            auto* def = ChordDatabase::getInstance().getChord (selectedChord.root, selectedChord.quality, targetInv);
                            if (def != nullptr)
                                selectedChord.name = def->name;
                            else
                                selectedChord.name = selectedChord.root + " " + selectedChord.quality;
                        }
                        else
                        {
                            selectedChord.customBassNote = targetBass;
                            selectedChord.inversion = 0;
                            selectedChord.name = selectedChord.root + " " + selectedChord.quality + "/" + targetBass;
                        }
                        
                        arrangement.sendProgressionToAudioThread();
                        arrangement.notifyChanges();
                    }
                }
            });
        }
        return;
    }
    else if (invButtonBounds.contains (pos))
    {
        isInversionDrawerActive = true;
        repaint();
        return;
    }

    // Grid Column click / drag initialization
    area.removeFromTop (10); // Spacer
    
    int totalWidth = area.getWidth();
    auto rootCol = area.removeFromLeft (static_cast<int> (totalWidth * 0.15f));
    area.removeFromLeft (8);
    auto durationCol = area.removeFromRight (static_cast<int> (totalWidth * 0.15f));
    area.removeFromRight (8);
    
    auto qualitiesArea = area;
    auto qualitiesClickArea = qualitiesArea;
    int qWidth = qualitiesArea.getWidth() / 3;
    auto qCol1 = qualitiesArea.removeFromLeft (qWidth);
    auto qCol2 = qualitiesArea.removeFromLeft (qWidth);
    auto qCol3 = qualitiesArea;
    
    // Check drag start column
    isDraggingRoots = false;
    isDraggingQualities = false;
    isDraggingDurations = false;
    
    if (rootCol.contains (pos))
    {
        isDraggingRoots = true;
        rootsScrollOffsetAtDragStart = rootsScrollOffset;
    }
    else if (durationCol.contains (pos))
    {
        isDraggingDurations = true;
        durationsScrollOffsetAtDragStart = durationsScrollOffset;
    }
    else if (qualitiesClickArea.contains (pos))
    {
        isDraggingQualities = true;
        qualitiesScrollOffsetAtDragStart = qualitiesScrollOffset;
    }

    if (selectedChord == nullptr)
        return;
        
    int cellHeight = 45;
    
    // Click on Roots Column
    if (rootCol.contains (pos))
    {
        juce::StringArray rootsList = {"C", "D", "E", "F", "G", "A", "B"};
        int idx = (pos.y - rootCol.getY() - rootsScrollOffset) / cellHeight;
        if (idx >= 0 && idx < rootsList.size())
        {
            int acc = getAccidentalIndex (selectedChord->root);
            selectedChord->root = combineRootAndAccidental (rootsList[idx], acc);
            selectedChord->name = selectedChord->root + " " + selectedChord->quality;
            arrangement.sendProgressionToAudioThread();
            arrangement.notifyChanges();
            updateAiAdvice();
        }
    }
    // Click on Durations Column
    else if (durationCol.contains (pos))
    {
        juce::StringArray durationsList = {"1/4", "2/4", "3/4", "4/4", "5/4", "6/4", "7/4", "7/8", "9/8", "11/8", "12/8"};
        int idx = (pos.y - durationCol.getY() - durationsScrollOffset) / cellHeight;
        if (idx >= 0 && idx < durationsList.size())
        {
            juce::String dur = durationsList[idx];
            int beats = 4;
            if (dur == "1/4") beats = 1;
            else if (dur == "2/4") beats = 2;
            else if (dur == "3/4") beats = 3;
            else if (dur == "4/4") beats = 4;
            else if (dur == "5/4") beats = 5;
            else if (dur == "6/4") beats = 6;
            else if (dur == "7/4") beats = 7;
            else if (dur == "7/8") beats = 2;
            else if (dur == "9/8") beats = 3;
            else if (dur == "11/8") beats = 4;
            else if (dur == "12/8") beats = 4;
            
            selectedChord->durationBeats = beats;
            selectedChord->durationText = dur;
            arrangement.sendProgressionToAudioThread();
            arrangement.notifyChanges();
            updateAiAdvice();
        }
    }
    // Click on Qualities Area
    else if (qualitiesClickArea.contains (pos))
    {
        juce::String clickedSuggestionOrQuality = "";
        
        if (isAdviceModeActive && currentCategory == "Common Chords")
        {
            if (qCol1.contains (pos))
            {
                int idx = (pos.y - qCol1.getY() - qualitiesScrollOffset) / cellHeight;
                if (idx >= 0 && idx < currentAdvice.safe.size())
                    clickedSuggestionOrQuality = currentAdvice.safe[idx];
            }
            else if (qCol2.contains (pos))
            {
                int idx = (pos.y - qCol2.getY() - qualitiesScrollOffset) / cellHeight;
                if (idx >= 0 && idx < currentAdvice.tension.size())
                    clickedSuggestionOrQuality = currentAdvice.tension[idx];
            }
            else if (qCol3.contains (pos))
            {
                int idx = (pos.y - qCol3.getY() - qualitiesScrollOffset) / cellHeight;
                if (idx >= 0 && idx < currentAdvice.experimental.size())
                    clickedSuggestionOrQuality = currentAdvice.experimental[idx];
            }
            
            if (clickedSuggestionOrQuality.isNotEmpty())
            {
                juce::StringArray tokens;
                tokens.addTokens (clickedSuggestionOrQuality, " ", "");
                if (tokens.size() > 0)
                {
                    selectedChord->root = tokens[0];
                    selectedChord->quality = tokens.size() > 1 ? tokens[1] : "Maj";
                    selectedChord->name = clickedSuggestionOrQuality;
                    arrangement.sendProgressionToAudioThread();
                    arrangement.notifyChanges();
                    updateAiAdvice();
                }
            }
        }
        else
        {
            juce::StringArray displayQualities = getQualitiesForCategory (currentCategory);
            int qCount = displayQualities.size();
            int colCount = (qCount + 2) / 3;
            
            juce::StringArray qList1, qList2, qList3;
            for (int i = 0; i < qCount; ++i)
            {
                if (i < colCount) qList1.add (displayQualities[i]);
                else if (i < colCount * 2) qList2.add (displayQualities[i]);
                else qList3.add (displayQualities[i]);
            }
            
            if (qCol1.contains (pos))
            {
                int idx = (pos.y - qCol1.getY() - qualitiesScrollOffset) / cellHeight;
                if (idx >= 0 && idx < qList1.size())
                    clickedSuggestionOrQuality = qList1[idx];
            }
            else if (qCol2.contains (pos))
            {
                int idx = (pos.y - qCol2.getY() - qualitiesScrollOffset) / cellHeight;
                if (idx >= 0 && idx < qList2.size())
                    clickedSuggestionOrQuality = qList2[idx];
            }
            else if (qCol3.contains (pos))
            {
                int idx = (pos.y - qCol3.getY() - qualitiesScrollOffset) / cellHeight;
                if (idx >= 0 && idx < qList3.size())
                    clickedSuggestionOrQuality = qList3[idx];
            }
            
            if (clickedSuggestionOrQuality.isNotEmpty())
            {
                selectedChord->quality = clickedSuggestionOrQuality;
                selectedChord->name = selectedChord->root + " " + selectedChord->quality;
                arrangement.sendProgressionToAudioThread();
                arrangement.notifyChanges();
                updateAiAdvice();
            }
        }
    }
}

void InspectorComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (bpmBounds.contains (event.getMouseDownPosition()))
    {
        int dragDistanceY = event.getMouseDownPosition().y - event.getPosition().y;
        float newBpm = initialDragBpm + static_cast<float> (dragDistanceY) * 0.5f;
        newBpm = juce::jlimit (20.0f, 300.0f, newBpm);
        arrangement.setTempo (newBpm);
    }
    else if (isDraggingRoots)
    {
        rootsScrollOffset = rootsScrollOffsetAtDragStart + event.getOffsetFromDragStart().y;
        constrainScrollOffsets();
        repaint();
    }
    else if (isDraggingDurations)
    {
        durationsScrollOffset = durationsScrollOffsetAtDragStart + event.getOffsetFromDragStart().y;
        constrainScrollOffsets();
        repaint();
    }
    else if (isDraggingQualities)
    {
        qualitiesScrollOffset = qualitiesScrollOffsetAtDragStart + event.getOffsetFromDragStart().y;
        constrainScrollOffsets();
        repaint();
    }
}

void InspectorComponent::mouseUp (const juce::MouseEvent& event)
{
    isDraggingRoots = false;
    isDraggingDurations = false;
    isDraggingQualities = false;

    if (bpmBounds.contains (event.getMouseDownPosition()) && ! event.mouseWasDraggedSinceMouseDown())
    {
        auto* alert = new juce::AlertWindow ("Change BPM", "Enter new BPM (20 - 300):", juce::AlertWindow::QuestionIcon);
        alert->addTextEditor ("bpmText", juce::String (arrangement.getBpm(), 1), "BPM:");
        
        auto* toggle = new juce::ToggleButton ("Apply globally to all sections");
        toggle->setToggleState (false, juce::dontSendNotification);
        toggle->setSize (250, 24);
        alert->addCustomComponent (toggle);
        
        alert->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        
        alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert, toggle] (int result)
        {
            if (result == 1)
            {
                auto text = alert->getTextEditorContents ("bpmText");
                float val = text.getFloatValue();
                if (val >= 20.0f && val <= 300.0f)
                {
                    bool applyGlobally = toggle->getToggleState();
                    arrangement.setTempo (val, applyGlobally);
                }
            }
            delete toggle;
            delete alert;
        }), true);
    }
}

