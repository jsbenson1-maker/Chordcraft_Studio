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
        for (int inv = 0; inv < 4; ++inv)
        {
            if (getChordBassNoteName (selectedChord->root, selectedChord->quality, inv) == targetBass)
            {
                selectedChord->inversion = inv;
                auto* def = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, inv);
                if (def != nullptr)
                    selectedChord->name = def->name;
                else
                    selectedChord->name = selectedChord->root + " " + selectedChord->quality;
                break;
            }
        }
    }

    juce::Colour getChordTheoryColour (const juce::String& activeKey, const juce::String& root, const juce::String& quality, juce::Colour fallbackColour = juce::Colour (0xffef4444))
    {
        juce::String keyRoot = "C";
        bool isMajorKey = true;
        
        juce::StringArray keyTokens;
        keyTokens.addTokens (activeKey, " ", "");
        if (keyTokens.size() > 0)
            keyRoot = keyTokens[0];
        if (keyTokens.size() > 1 && keyTokens[1].equalsIgnoreCase ("Min"))
            isMajorKey = false;

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

        int keyPitch = getPitchClass (keyRoot);
        int rootPitch = getPitchClass (root);
        int diff = (rootPitch - keyPitch + 12) % 12;

        if (isMajorKey)
        {
            if (diff == 0) // I
            {
                if (quality == "Maj" || quality == "Maj7" || quality == "6" || quality == "Maj9" || quality == "Maj11" || quality == "Maj13" || quality == "Sus2" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 2) // ii
            {
                if (quality == "Min" || quality == "Min7" || quality == "Min9" || quality == "Min11" || quality == "Min13" || quality == "Sus2" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 4) // iii
            {
                if (quality == "Min" || quality == "Min7" || quality == "Min11" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 5) // IV
            {
                if (quality == "Maj" || quality == "Maj7" || quality == "6" || quality == "Maj9" || quality == "Maj11" || quality == "Maj13" || quality == "Sus2" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 7) // V
            {
                if (quality == "Maj" || quality == "7" || quality == "9" || quality == "11" || quality == "13" || quality == "Sus2" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 9) // vi
            {
                if (quality == "Min" || quality == "Min7" || quality == "Min9" || quality == "Min11" || quality == "Min13" || quality == "Sus2" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 11) // vii°
            {
                if (quality == "Dim" || quality == "m7b5" || quality == "5")
                    return juce::Colour (0xff10b981);
            }

            // Modal Mixtures (Yellow)
            if (diff == 0 && (quality == "Min" || quality == "Min7" || quality == "Min9")) return juce::Colour (0xfff59e0b);
            if (diff == 2 && (quality == "Maj" || quality == "7" || quality == "9" || quality == "13")) return juce::Colour (0xfff59e0b);
            if (diff == 4 && (quality == "Maj" || quality == "7" || quality == "9")) return juce::Colour (0xfff59e0b);
            if (diff == 9 && (quality == "Maj" || quality == "7")) return juce::Colour (0xfff59e0b);
            if (diff == 11 && (quality == "Maj" || quality == "7")) return juce::Colour (0xfff59e0b);
            if (diff == 0 && quality == "7") return juce::Colour (0xfff59e0b);

            if (diff == 3 && (quality == "Maj" || quality == "Maj7" || quality == "6")) return juce::Colour (0xfff59e0b);
            if (diff == 5 && (quality == "Min" || quality == "Min7" || quality == "Min9")) return juce::Colour (0xfff59e0b);
            if (diff == 7 && (quality == "Min" || quality == "Min7")) return juce::Colour (0xfff59e0b);
            if (diff == 8 && (quality == "Maj" || quality == "Maj7" || quality == "6")) return juce::Colour (0xfff59e0b);
            if (diff == 10 && (quality == "Maj" || quality == "Maj7" || quality == "6" || quality == "7")) return juce::Colour (0xfff59e0b);

            if (diff == 1 && (quality == "7" || quality == "9" || quality == "13")) return juce::Colour (0xfff59e0b);
        }
        else
        {
            if (diff == 0) // i
            {
                if (quality == "Min" || quality == "Min7" || quality == "Min9" || quality == "Min11" || quality == "Sus2" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 2) // ii°
            {
                if (quality == "Dim" || quality == "m7b5" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 3) // III
            {
                if (quality == "Maj" || quality == "Maj7" || quality == "6" || quality == "Maj9" || quality == "Sus2" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 5) // iv
            {
                if (quality == "Min" || quality == "Min7" || quality == "Min9" || quality == "Min11" || quality == "Sus2" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 7) // v / V
            {
                if (quality == "Min" || quality == "Min7" || quality == "Maj" || quality == "7" || quality == "9" || quality == "Sus2" || quality == "Sus4" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 8) // VI
            {
                if (quality == "Maj" || quality == "Maj7" || quality == "6" || quality == "Sus2" || quality == "5")
                    return juce::Colour (0xff10b981);
            }
            else if (diff == 10) // VII
            {
                if (quality == "Maj" || quality == "7" || quality == "9" || quality == "Sus2" || quality == "5")
                    return juce::Colour (0xff10b981);
            }

            // Modal Mixtures (Yellow)
            if (diff == 0 && (quality == "Maj" || quality == "7" || quality == "Maj7")) return juce::Colour (0xfff59e0b);
            if (diff == 5 && (quality == "Maj" || quality == "7")) return juce::Colour (0xfff59e0b);
            if (diff == 2 && (quality == "Min" || quality == "Min7")) return juce::Colour (0xfff59e0b);
            if (diff == 2 && (quality == "Maj" || quality == "7")) return juce::Colour (0xfff59e0b);
            if (diff == 11 && (quality == "Dim" || quality == "m7b5")) return juce::Colour (0xfff59e0b);
        }

        return fallbackColour;
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
    playButtonBounds = playBar.removeFromLeft(static_cast<int>(playWidth * 0.45f));
    exportBounds = playBar.removeFromLeft(static_cast<int>(playWidth * 0.25f));
    bpmBounds = playBar;
    
    juce::String playText = arrangement.isPlaying() ? 
        juce::String::fromUTF8("\xe2\x96\xaa  STOP") : 
        juce::String::fromUTF8("\xe2\x96\xb6  PLAY");
    
    drawHardwareButton(g, playButtonBounds, playText, juce::Colour(0xff3b82f6));
    drawHardwareButton(g, exportBounds, "EXPORT", juce::Colour(0xff8b5cf6));
    
    juce::String bpmText = "BPM: " + juce::String(arrangement.getBpm(), 1);
    drawHardwareButton(g, bpmBounds, bpmText, juce::Colour(0xff10b981));
    
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
    juce::String bassText = "Bass: " + (selectedChord ? getChordBassNoteName (selectedChord->root, selectedChord->quality, selectedChord->inversion) : "C");
    juce::Colour invColor = (selectedChord && selectedChord->inversion > 0) ? ThemeManager::getSystemAccentColor() : btnGrey;
    
    drawHardwareButton(g, keyButtonBounds, keyText, btnGrey);
    drawHardwareButton(g, bassButtonBounds, bassText, btnGrey);
    drawHardwareButton(g, categoryButtonBounds, currentCategory, ThemeManager::getSystemAccentColor());
    drawHardwareButton(g, invButtonBounds, "INV", invColor);

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

    // Draw roots and durations
    drawGridCol (rootCol, {"C", "D", "E", "F", "G", "A", "B"}, juce::Colour(0xff94a3b8), false, gridScrollOffset); 
    drawGridCol (durationCol, allDurations, juce::Colour(0xff8b5cf6), false, gridScrollOffset);

    // Dynamic qualities list based on category & advice
    if (isAdviceModeActive && currentCategory == "Common Chords")
    {
        drawGridCol (qCol1, currentAdvice.safe, juce::Colour(0xff10b981), true, gridScrollOffset); 
        drawGridCol (qCol2, currentAdvice.tension, juce::Colour(0xfff59e0b), true, gridScrollOffset); 
        drawGridCol (qCol3, currentAdvice.experimental, juce::Colour(0xffef4444), true, gridScrollOffset); 
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
        
        drawGridCol (qCol1, qList1, juce::Colour(0xff10b981), true, gridScrollOffset); 
        drawGridCol (qCol2, qList2, juce::Colour(0xfff59e0b), true, gridScrollOffset); 
        drawGridCol (qCol3, qList3, juce::Colour(0xffef4444), true, gridScrollOffset); 
    }

    g.restoreState();;
}

void InspectorComponent::resized() {}

void InspectorComponent::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    gridScrollOffset += static_cast<int>(wheel.deltaY * 100.0f);
    if (gridScrollOffset > 0) gridScrollOffset = 0;
    
    juce::StringArray displayQualities = getQualitiesForCategory(currentCategory);
    int qCount = displayQualities.size();
    int colCount = (qCount + 2) / 3;
    int totalHeightNeeded = colCount * 45;
    
    auto area = getLocalBounds().reduced (12);
    area.removeFromBottom (45);
    area.removeFromBottom (10);
    area.removeFromTop (45);
    area.removeFromTop (45);
    area.removeFromTop (10);
    
    int maxScroll = - (totalHeightNeeded - area.getHeight() + 20);
    if (maxScroll > 0) maxScroll = 0;
    
    if (gridScrollOffset < maxScroll) gridScrollOffset = maxScroll;
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
    playButtonBounds = playBar.removeFromLeft (static_cast<int> (playWidth * 0.45f));
    exportBounds = playBar.removeFromLeft (static_cast<int> (playWidth * 0.25f));
    bpmBounds = playBar;
    
    area.removeFromBottom (10);
    
    if (! arrangement.isClipboardModeActive)
    {
        auto topRow1 = area.removeFromTop (45);
        auto topRow2 = area.removeFromTop (45);
        
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
    }
    
    // Bottom transport / export
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
    else if (exportBounds.contains (pos))
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Export Multitrack MIDI (.mid)");
        menu.addItem (2, "Export Offline Audio (.wav)");
        
        menu.showMenuAsync (juce::PopupMenu::Options(), [this](int result)
        {
            if (result == 1)
            {
                performMidiExport (arrangement.getChords(), arrangement.getBpm());
            }
            else if (result == 2)
            {
                if (exportThread != nullptr && exportThread->isThreadRunning())
                {
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon,
                        "Export in Progress",
                        "An offline render export is already running in the background.",
                        "OK"
                    );
                }
                else
                {
                    exportThread = std::make_unique<OfflineRenderThread> (arrangement.getChords(), arrangement.getBpm());
                    exportThread->startThread();
                }
            }
        });
        return;
    }

    // Accidentals buttons
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
            
            gridScrollOffset = 0;
            repaint();
        });
        return;
    }

    // Top action row
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
        juce::StringArray keysList = { "C Maj", "Db Maj", "D Maj", "Eb Maj", "E Maj", "F Maj", "Gb Maj", "G Maj", "Ab Maj", "A Maj", "Bb Maj", "B Maj", "A Min", "E Min", "D Min", "G Min" };
        for (int i = 0; i < keysList.size(); ++i)
        {
            menu.addItem (i + 1, keysList[i], true, arrangement.activeKey == keysList[i]);
        }
        
        menu.showMenuAsync (juce::PopupMenu::Options(), [this, keysList](int result)
        {
            if (result >= 1 && result <= keysList.size())
            {
                arrangement.activeKey = keysList[result - 1];
                arrangement.notifyChanges();
                updateAiAdvice();
            }
        });
        return;
    }
    else if (bassButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        ChordBlock* selectedChord = nullptr;
        for (auto& cb : chords)
        {
            if (cb.isSelected) { selectedChord = &cb; break; }
        }
        
        if (selectedChord != nullptr)
        {
            juce::PopupMenu menu;
            juce::StringArray availableBassNotes;
            juce::Array<int> inversionMap;
            
            for (int inv = 0; inv < 4; ++inv)
            {
                auto* def = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, inv);
                if (def != nullptr)
                {
                    juce::String bassName = getChordBassNoteName (selectedChord->root, selectedChord->quality, inv);
                    if (! availableBassNotes.contains (bassName))
                    {
                        availableBassNotes.add (bassName);
                        inversionMap.add (inv);
                    }
                }
            }
            
            juce::String currentBass = getChordBassNoteName (selectedChord->root, selectedChord->quality, selectedChord->inversion);
            
            for (int i = 0; i < availableBassNotes.size(); ++i)
            {
                menu.addItem (i + 1, availableBassNotes[i], true, currentBass == availableBassNotes[i]);
            }
            
            menu.showMenuAsync (juce::PopupMenu::Options(), [this, selectedChord, availableBassNotes, inversionMap](int result)
            {
                if (result >= 1 && result <= availableBassNotes.size())
                {
                    int targetInv = inversionMap[result - 1];
                    selectedChord->inversion = targetInv;
                    
                    auto* def = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, targetInv);
                    if (def != nullptr)
                        selectedChord->name = def->name;
                    else
                        selectedChord->name = selectedChord->root + " " + selectedChord->quality;
                        
                    arrangement.sendProgressionToAudioThread();
                    arrangement.notifyChanges();
                }
            });
        }
        return;
    }
    else if (invButtonBounds.contains (pos))
    {
        auto& chords = arrangement.getChords();
        ChordBlock* selectedChord = nullptr;
        for (auto& cb : chords)
        {
            if (cb.isSelected) { selectedChord = &cb; break; }
        }
        
        if (selectedChord != nullptr)
        {
            int nextInv = (selectedChord->inversion + 1) % 4;
            auto* testChord = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, nextInv);
            if (testChord != nullptr)
            {
                selectedChord->inversion = nextInv;
            }
            else
            {
                selectedChord->inversion = 0; // Wrap back to root position
            }
            
            auto* def = ChordDatabase::getInstance().getChord (selectedChord->root, selectedChord->quality, selectedChord->inversion);
            if (def != nullptr)
                selectedChord->name = def->name;
            else
                selectedChord->name = selectedChord->root + " " + selectedChord->quality;
                
            arrangement.sendProgressionToAudioThread();
            arrangement.notifyChanges();
            updateAiAdvice();
        }
        return;
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

    // Grid Column Clicks
    area = getLocalBounds().reduced (12);
    area.removeFromBottom (45);
    area.removeFromBottom (10);
    area.removeFromTop (45);
    area.removeFromTop (45);
    area.removeFromTop (10);

    int totalWidth = area.getWidth();
    auto rootCol = area.removeFromLeft (static_cast<int> (totalWidth * 0.15f));
    area.removeFromLeft (8);
    auto durationCol = area.removeFromRight (static_cast<int> (totalWidth * 0.15f));
    area.removeFromRight (8);

    auto qualitiesArea = area;
    auto qualitiesClickArea = qualitiesArea; // Save it before mutating!
    int qWidth = qualitiesArea.getWidth() / 3;
    auto qCol1 = qualitiesArea.removeFromLeft (qWidth);
    auto qCol2 = qualitiesArea.removeFromLeft (qWidth);
    auto qCol3 = qualitiesArea;

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

    if (selectedChord == nullptr)
        return;

    int cellHeight = 45;

    // Click on Roots Column
    // Click on Roots Column
    if (rootCol.contains (pos))
    {
        juce::StringArray rootsList = {"C", "D", "E", "F", "G", "A", "B"};
        int idx = (pos.y - rootCol.getY() - gridScrollOffset) / cellHeight;
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
        int idx = (pos.y - durationCol.getY() - gridScrollOffset) / cellHeight;
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
            selectedChord->durationText = dur; // Apply duration text to chord block!
            arrangement.sendProgressionToAudioThread();
            arrangement.notifyChanges();
            updateAiAdvice();
        }
    }
    // Click on Qualities / Suggestions Area
    else if (qualitiesClickArea.contains (pos))
    {
        juce::String clickedSuggestionOrQuality = "";
        
        if (isAdviceModeActive && currentCategory == "Common Chords")
        {
            if (qCol1.contains (pos))
            {
                int idx = (pos.y - qCol1.getY() - gridScrollOffset) / cellHeight;
                if (idx >= 0 && idx < currentAdvice.safe.size())
                    clickedSuggestionOrQuality = currentAdvice.safe[idx];
            }
            else if (qCol2.contains (pos))
            {
                int idx = (pos.y - qCol2.getY() - gridScrollOffset) / cellHeight;
                if (idx >= 0 && idx < currentAdvice.tension.size())
                    clickedSuggestionOrQuality = currentAdvice.tension[idx];
            }
            else if (qCol3.contains (pos))
            {
                int idx = (pos.y - qCol3.getY() - gridScrollOffset) / cellHeight;
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
                int idx = (pos.y - qCol1.getY() - gridScrollOffset) / cellHeight;
                if (idx >= 0 && idx < qList1.size())
                    clickedSuggestionOrQuality = qList1[idx];
            }
            else if (qCol2.contains (pos))
            {
                int idx = (pos.y - qCol2.getY() - gridScrollOffset) / cellHeight;
                if (idx >= 0 && idx < qList2.size())
                    clickedSuggestionOrQuality = qList2[idx];
            }
            else if (qCol3.contains (pos))
            {
                int idx = (pos.y - qCol3.getY() - gridScrollOffset) / cellHeight;
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
}

void InspectorComponent::mouseUp (const juce::MouseEvent& event)
{
    if (bpmBounds.contains (event.getMouseDownPosition()) && ! event.mouseWasDraggedSinceMouseDown())
    {
        auto* alert = new juce::AlertWindow ("Change BPM", "Enter new BPM (20 - 300):", juce::AlertWindow::QuestionIcon);
        alert->addTextEditor ("bpmText", juce::String (arrangement.getBpm(), 1), "BPM:");
        alert->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        
        alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert] (int result)
        {
            if (result == 1)
            {
                auto text = alert->getTextEditorContents ("bpmText");
                float val = text.getFloatValue();
                if (val >= 20.0f && val <= 300.0f)
                {
                    arrangement.setTempo (val);
                }
            }
            delete alert;
        }), true);
    }
}

