#include "GlobalMenuComponent.h"
#include "ThemeManager.h"
#include "ExportEngine.h"
#include "AiCoreManager.h"



GlobalMenuComponent::GlobalMenuComponent (ChordArrangement& arr)
    : arrangement (arr)
{
    setSize (600, 500);
}

void GlobalMenuComponent::paint (juce::Graphics& g)
{
    // Draw background panel (Dark Purple/Navy)
    g.setColour (juce::Colour (0xff1e1e2e));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    // Glowing border
    g.setColour (ThemeManager::getSystemAccentColor());
    g.drawRoundedRectangle (getLocalBounds().toFloat(), 8.0f, 2.0f);

    // Render Sub-Views or Main View
    if (isTransposeViewActive)
    {
        // Transpose header
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions ("Georgia", 18.0f, juce::Font::bold));
        g.drawText ("TRANSPOSE ARRANGEMENT", 0, 0, getWidth(), headerHeight, juce::Justification::centred);

        g.setColour (juce::Colour (0xff3f3f5a));
        g.drawHorizontalLine (headerHeight - 2, 10.0f, getWidth() - 10.0f);

        // Draw sub-grid transpose buttons (+/- 1 to 6)
        for (int i = 0; i < 6; ++i)
        {
            drawHardwareButton (g, transposeLeft[i], "+" + juce::String (i + 1) + " Semitone" + (i > 0 ? "s" : ""), ThemeManager::getSystemAccentColor());
            drawHardwareButton (g, transposeRight[i], "-" + juce::String (i + 1) + " Semitone" + (i > 0 ? "s" : ""), juce::Colour (0xffef4444));
        }

        // Draw Back button
        drawHardwareButton (g, transposeBackBtnBounds, "< BACK", juce::Colour (0xff475569));
    }
    else if (isSettingsViewActive)
    {
        // Settings header
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions ("Georgia", 18.0f, juce::Font::bold));
        g.drawText ("APP SETTINGS", 0, 0, getWidth(), headerHeight, juce::Justification::centred);

        g.setColour (juce::Colour (0xff3f3f5a));
        g.drawHorizontalLine (headerHeight - 2, 10.0f, getWidth() - 10.0f);

        // Draw Settings Toggles
        g.setFont (14.0f);
        g.setColour (juce::Colours::white);
        
        // Loop Song
        g.drawText ("Loop Arrangement Playback", 30, loopToggleBounds.getY(), 250, loopToggleBounds.getHeight(), juce::Justification::centredLeft);
        drawHardwareButton (g, loopToggleBounds, arrangement.isLoopingEnabled ? "ON" : "OFF", arrangement.isLoopingEnabled ? ThemeManager::getSystemAccentColor() : juce::Colour (0xff4b5563));

        // Chord Preview
        g.drawText ("Audition Chords on Grid Tap", 30, previewToggleBounds.getY(), 250, previewToggleBounds.getHeight(), juce::Justification::centredLeft);
        drawHardwareButton (g, previewToggleBounds, arrangement.isChordPreviewEnabled ? "ON" : "OFF", arrangement.isChordPreviewEnabled ? ThemeManager::getSystemAccentColor() : juce::Colour (0xff4b5563));

        // Draw Back button
        drawHardwareButton (g, settingsBackBtnBounds, "< BACK", juce::Colour (0xff475569));
    }
    else
    {
        // Main view header
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions ("Georgia", 18.0f, juce::Font::bold));
        g.drawText ("FILE / TOOLS", 30, 8, getWidth() - 60, 25, juce::Justification::centred);

        g.setFont (juce::FontOptions ("Georgia", 12.0f, juce::Font::italic));
        g.setColour (juce::Colours::grey);
        g.drawText ("Song: " + arrangement.songName, 30, 31, getWidth() - 60, 20, juce::Justification::centred);

        g.setColour (juce::Colour (0xff3f3f5a));
        g.drawHorizontalLine (headerHeight - 2, 10.0f, getWidth() - 10.0f);

        // Main Grid Draw
        drawHardwareButton (g, mainGrid.leftCol[0], "OPEN SONG", juce::Colour (0xff0284c7));      // Open
        drawHardwareButton (g, mainGrid.leftCol[1], "SAVE SONG", juce::Colour (0xff059669));      // Save
        drawHardwareButton (g, mainGrid.leftCol[2], "EXPORT AUDIO/MIDI", juce::Colour (0xff7c3aed)); // Export
        drawHardwareButton (g, mainGrid.leftCol[3], "NEW PROGRESSION", juce::Colour (0xffdc2626));   // New

        drawHardwareButton (g, mainGrid.rightCol[0], "TRANSPOSE", juce::Colour (0xffd97706));        // Transpose
        drawHardwareButton (g, mainGrid.rightCol[1], "AUTO-COMPOSER", ThemeManager::getSystemAccentColor()); // Auto-Composer
        drawHardwareButton (g, mainGrid.rightCol[2], "COPY / PASTE DRAWERS", juce::Colour (0xff0284c7)); // Clipboard
        drawHardwareButton (g, mainGrid.rightCol[3], "HELP MANUAL", juce::Colour (0xff4b5563));     // Help

        // Footer buttons
        g.setColour (juce::Colour (0xff3f3f5a));
        g.drawHorizontalLine (getHeight() - footerHeight + 2, 10.0f, getWidth() - 10.0f);

        drawHardwareButton (g, settingsBtnBounds, "SETTINGS", juce::Colour (0xff4b5563));
        drawHardwareButton (g, closeBtnBounds, "CLOSE MENU", ThemeManager::getSystemAccentDark());
    }
}

void GlobalMenuComponent::resized()
{
    int w = getWidth();
    int h = getHeight();
    int padding = 15;

    // 1. Layout Main View Grid (2 columns, 4 rows)
    int gridY = headerHeight + 10;
    int gridH = h - headerHeight - footerHeight - 20;
    int rowH = gridH / 4;
    int colW = (w - (padding * 3)) / 2;

    for (int r = 0; r < 4; ++r)
    {
        int y = gridY + r * rowH;
        mainGrid.leftCol[r] = juce::Rectangle<int> (padding, y + 4, colW, rowH - 8);
        mainGrid.rightCol[r] = juce::Rectangle<int> (padding + colW + padding, y + 4, colW, rowH - 8);
    }

    // Main View Footer
    int footerBtnW = 140;
    int footerBtnH = 32;
    settingsBtnBounds = juce::Rectangle<int> (padding, h - footerHeight + (footerHeight - footerBtnH) / 2, footerBtnW, footerBtnH);
    closeBtnBounds = juce::Rectangle<int> (w - padding - footerBtnW, h - footerHeight + (footerHeight - footerBtnH) / 2, footerBtnW, footerBtnH);

    // 2. Layout Transpose View (2 columns, 6 rows)
    int tGridY = headerHeight + 10;
    int tGridH = h - headerHeight - 30;
    int tRowH = tGridH / 6;
    int tColW = (w - (padding * 3)) / 2;
    for (int r = 0; r < 6; ++r)
    {
        int y = tGridY + r * tRowH;
        transposeLeft[r] = juce::Rectangle<int> (padding, y + 3, tColW, tRowH - 6);
        transposeRight[r] = juce::Rectangle<int> (padding + tColW + padding, y + 3, tColW, tRowH - 6);
    }
    transposeBackBtnBounds = juce::Rectangle<int> (padding, 12, 70, 30);

    // 3. Layout Settings View
    settingsBackBtnBounds = juce::Rectangle<int> (padding, 12, 70, 30);
    int toggleW = 80;
    int toggleH = 34;
    loopToggleBounds = juce::Rectangle<int> (w - padding - toggleW, headerHeight + 30, toggleW, toggleH);
    previewToggleBounds = juce::Rectangle<int> (w - padding - toggleW, headerHeight + 90, toggleW, toggleH);
}

void GlobalMenuComponent::mouseDown (const juce::MouseEvent& event)
{
    startDragX = event.position.getX();
    startDragY = event.position.getY();
    hasSwipedDismiss = false;

    auto pos = event.getPosition();

    if (isTransposeViewActive)
    {
        // Check back button
        if (transposeBackBtnBounds.contains (pos))
        {
            ThemeManager::triggerHapticImpact();
            isTransposeViewActive = false;
            repaint();
            return;
        }

        // Check transpose buttons
        for (int i = 0; i < 6; ++i)
        {
            if (transposeLeft[i].contains (pos))
            {
                ThemeManager::triggerHapticRatchet();
                transposeArrangement (i + 1);
                isTransposeViewActive = false;
                repaint();
                return;
            }
            if (transposeRight[i].contains (pos))
            {
                ThemeManager::triggerHapticRatchet();
                transposeArrangement (- (i + 1));
                isTransposeViewActive = false;
                repaint();
                return;
            }
        }
        return;
    }

    if (isSettingsViewActive)
    {
        // Check back button
        if (settingsBackBtnBounds.contains (pos))
        {
            ThemeManager::triggerHapticImpact();
            isSettingsViewActive = false;
            repaint();
            return;
        }

        // Check Loop Toggle
        if (loopToggleBounds.contains (pos))
        {
            ThemeManager::triggerHapticRatchet();
            arrangement.isLoopingEnabled = ! arrangement.isLoopingEnabled;
            repaint();
            return;
        }

        // Check Preview Toggle
        if (previewToggleBounds.contains (pos))
        {
            ThemeManager::triggerHapticRatchet();
            arrangement.isChordPreviewEnabled = ! arrangement.isChordPreviewEnabled;
            repaint();
            return;
        }
        return;
    }

    // 1. Check main grid buttons
    // Left column
    if (mainGrid.leftCol[0].contains (pos)) // Open
    {
        ThemeManager::triggerHapticImpact();
        performOpen();
        return;
    }
    if (mainGrid.leftCol[1].contains (pos)) // Save
    {
        ThemeManager::triggerHapticImpact();
        performSave();
        return;
    }
    if (mainGrid.leftCol[2].contains (pos)) // Export
    {
        ThemeManager::triggerHapticImpact();
        performExport();
        return;
    }
    if (mainGrid.leftCol[3].contains (pos)) // New
    {
        ThemeManager::triggerHapticImpact();
        performNew();
        return;
    }

    // Right column
    if (mainGrid.rightCol[0].contains (pos)) // Transpose
    {
        ThemeManager::triggerHapticImpact();
        isTransposeViewActive = true;
        repaint();
        return;
    }
    if (mainGrid.rightCol[1].contains (pos)) // Auto-Composer
    {
        ThemeManager::triggerHapticImpact();
        performAutoComposer();
        return;
    }
    if (mainGrid.rightCol[2].contains (pos)) // Copy / Paste Drawers (Clipboard Mode)
    {
        ThemeManager::triggerHapticImpact();
        arrangement.isClipboardModeActive = true;
        arrangement.notifyChanges();
        if (onCloseClicked != nullptr)
            onCloseClicked();
        return;
    }
    if (mainGrid.rightCol[3].contains (pos)) // Help
    {
        ThemeManager::triggerHapticImpact();
        performHelp();
        return;
    }

    // 2. Check footer buttons
    if (settingsBtnBounds.contains (pos))
    {
        ThemeManager::triggerHapticImpact();
        isSettingsViewActive = true;
        repaint();
        return;
    }

    if (closeBtnBounds.contains (pos))
    {
        ThemeManager::triggerHapticImpact();
        if (onCloseClicked != nullptr)
            onCloseClicked();
    }
}

void GlobalMenuComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (hasSwipedDismiss) return;
    float deltaX = event.position.getX() - startDragX;
    float deltaY = event.position.getY() - startDragY;
    
    if (std::abs (deltaX) > 60.0f && std::abs (deltaY) < 30.0f)
    {
        if (startDragX < 40.0f && deltaX > 80.0f) // iOS left-edge swipe
        {
            hasSwipedDismiss = true;
            ThemeManager::triggerHapticImpact();
            if (onCloseClicked != nullptr)
                onCloseClicked();
        }
    }
}

bool GlobalMenuComponent::keyPressed (const juce::KeyPress& key)
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

void GlobalMenuComponent::visibilityChanged()
{
    if (isVisible())
    {
        setWantsKeyboardFocus (true);
        grabKeyboardFocus();
    }
}

void GlobalMenuComponent::drawHardwareButton (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& text, juce::Colour baseColour)
{
    g.setColour (baseColour.darker (0.2f));
    g.fillRoundedRectangle (bounds.toFloat(), 5.0f);

    g.setColour (baseColour.brighter (0.1f));
    g.drawRoundedRectangle (bounds.toFloat(), 5.0f, 1.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (text, bounds, juce::Justification::centred, true);
}

void GlobalMenuComponent::transposeArrangement (int semitones)
{
    juce::StringArray pitches = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    auto getPitchClass = [&](const juce::String& r) {
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

    auto& chords = arrangement.getChords();
    for (auto& cb : chords)
    {
        int currentPitch = getPitchClass (cb.root);
        int newPitch = (currentPitch + semitones + 12) % 12;
        cb.root = pitches[newPitch];

        // Resolve clean inversion names
        auto* def = ChordDatabase::getInstance().getChord (cb.root, cb.quality, cb.inversion);
        if (def != nullptr)
            cb.name = def->name;
        else
            cb.name = cb.root + " " + cb.quality;
    }

    arrangement.sendProgressionToAudioThread();
    arrangement.notifyChanges();

    juce::AlertWindow::showMessageBoxAsync (
        juce::AlertWindow::InfoIcon,
        "Transposed Successful",
        "Arrangement mathematically shifted by " + juce::String (semitones) + " semitones.",
        "OK"
    );
}

void GlobalMenuComponent::performNew()
{
    auto* alert = new juce::AlertWindow ("New Progression", "Create a new song progression? All current chord blocks will be cleared.", juce::AlertWindow::QuestionIcon);
    alert->addButton ("Create New", 1);
    alert->addButton ("Cancel", 0);
    alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert](int result)
    {
        if (result == 1)
        {
            arrangement.getChords().clear();
            arrangement.getChords().add ({ "C Maj", "C", "Maj", 0, 4, "4/4", juce::Colour (0xff0f172a), true, {}, {}, 1 });
            arrangement.setTempo (120.0f);

            // Default lanes configuration
            for (int i = 0; i < 12; ++i)
                arrangement.trackLanes[i] = { true, 0, "" }; // Piano, no pattern
            arrangement.trackLanes[12] = { true, 0, "drums_standard_rock_8th_hats" }; // Drum lane enabled

            arrangement.songName = "Untitled";
            arrangement.sendProgressionToAudioThread();
            arrangement.notifyChanges();

            if (onCloseClicked != nullptr)
                onCloseClicked();
        }
        delete alert;
    }), true);
}

void GlobalMenuComponent::performSave()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Save Chordcraft Project",
        getExportDirectory().getChildFile (arrangement.songName + ".chordcraft"),
        "*.chordcraft"
    );

    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.getFileName().isNotEmpty())
            {
                if (file.getFileExtension() != ".chordcraft")
                    file = file.withFileExtension (".chordcraft");

                // Construct JSON serialization
                juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                obj->setProperty ("songName", file.getFileNameWithoutExtension());
                obj->setProperty ("bpm", arrangement.getBpm());
                obj->setProperty ("activeKey", arrangement.activeKey);

                juce::var chordsArray;
                for (auto& cb : arrangement.getChords())
                {
                    juce::DynamicObject::Ptr c = new juce::DynamicObject();
                    c->setProperty ("name", cb.name);
                    c->setProperty ("root", cb.root);
                    c->setProperty ("quality", cb.quality);
                    c->setProperty ("inversion", cb.inversion);
                    c->setProperty ( "durationBeats", cb.durationBeats);
                    c->setProperty ( "durationText", cb.durationText);
                    c->setProperty ( "octave", cb.octave);
                    c->setProperty ( "customBassNote", cb.customBassNote);
                    chordsArray.append (c.get());
                }
                obj->setProperty ("chords", chordsArray);

                juce::var lanesArray;
                for (auto& lane : arrangement.trackLanes)
                {
                    juce::DynamicObject::Ptr l = new juce::DynamicObject();
                    l->setProperty ("enabled", lane.enabled);
                    l->setProperty ("gmProgramNumber", lane.gmProgramNumber);
                    l->setProperty ("patternId", lane.patternId);
                    lanesArray.append (l.get());
                }
                obj->setProperty ("trackLanes", lanesArray);

                juce::var document (obj.get());
                juce::String jsonText = juce::JSON::toString (document);

                if (file.existsAsFile())
                    file.deleteFile();

                std::unique_ptr<juce::FileOutputStream> stream = file.createOutputStream();
                if (stream != nullptr && stream->openedOk())
                {
                    stream->writeString (jsonText);
                    stream.reset();

                    arrangement.songName = file.getFileNameWithoutExtension();
                    repaint();

                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::InfoIcon,
                        "Save Successful",
                        "Project saved to Documents:\n" + file.getFullPathName(),
                        "OK"
                    );
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon,
                        "Save Failed",
                        "Could not open/write the output stream.",
                        "OK"
                    );
                }
            }
        });
}

void GlobalMenuComponent::performOpen()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Open Chordcraft Project",
        getExportDirectory(),
        "*.chordcraft"
    );

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                juce::String jsonText = file.loadFileAsString();
                auto parsed = juce::JSON::parse (jsonText);

                if (parsed.isObject())
                {
                    auto* obj = parsed.getDynamicObject();
                    if (obj != nullptr)
                    {
                        arrangement.songName = obj->getProperty ("songName").toString();
                        if (arrangement.songName.isEmpty())
                            arrangement.songName = file.getFileNameWithoutExtension();

                        float bpm = static_cast<float> (obj->getProperty ("bpm"));
                        if (bpm >= 20.0f && bpm <= 300.0f)
                            arrangement.setTempo (bpm);

                        arrangement.activeKey = obj->getProperty ("activeKey").toString();
                        if (arrangement.activeKey.isEmpty())
                            arrangement.activeKey = "C Maj";

                        auto chordsVar = obj->getProperty ("chords");
                        if (auto* chordsArr = chordsVar.getArray())
                        {
                            auto& chords = arrangement.getChords();
                            chords.clear();
                            for (auto& var : *chordsArr)
                            {
                                if (auto* cObj = var.getDynamicObject())
                                {
                                    ChordBlock cb;
                                    cb.root = cObj->getProperty ("root").toString();
                                    cb.quality = cObj->getProperty ("quality").toString();
                                    cb.inversion = static_cast<int> (cObj->getProperty ("inversion"));
                                    cb.durationBeats = static_cast<int> (cObj->getProperty ("durationBeats"));
                                    cb.durationText = cObj->getProperty ("durationText").toString();
                                    cb.octave = static_cast<int> (cObj->getProperty ("octave"));
                                    cb.customBassNote = cObj->getProperty ("customBassNote").toString();

                                    auto* def = ChordDatabase::getInstance().getChord (cb.root, cb.quality, cb.inversion);
                                    if (cb.customBassNote.isNotEmpty())
                                        cb.name = cb.root + " " + cb.quality + "/" + cb.customBassNote;
                                    else if (def != nullptr)
                                        cb.name = def->name;
                                    else
                                        cb.name = cb.root + " " + cb.quality;

                                    cb.colour = juce::Colour (0xff0f172a);
                                    cb.isSelected = false;
                                    chords.add (cb);
                                }
                            }
                            if (chords.isEmpty())
                                chords.add ({ "C Maj", "C", "Maj", 0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
                            else
                                chords.getReference (0).isSelected = true;
                        }

                        auto lanesVar = obj->getProperty ("trackLanes");
                        if (auto* lanesArr = lanesVar.getArray())
                        {
                            int count = juce::jmin (13, (int) lanesArr->size());
                            for (int i = 0; i < count; ++i)
                            {
                                if (auto* lObj = (*lanesArr)[i].getDynamicObject())
                                {
                                    arrangement.trackLanes[i].enabled = static_cast<bool> (lObj->getProperty ("enabled"));
                                    arrangement.trackLanes[i].gmProgramNumber = static_cast<int> (lObj->getProperty ("gmProgramNumber"));
                                    arrangement.trackLanes[i].patternId = lObj->getProperty ("patternId").toString();
                                }
                            }
                        }

                        arrangement.sendProgressionToAudioThread();
                        arrangement.notifyChanges();

                        juce::AlertWindow::showMessageBoxAsync (
                            juce::AlertWindow::InfoIcon,
                            "Open Successful",
                            "Loaded project:\n" + arrangement.songName,
                            "OK"
                        );

                        if (onCloseClicked != nullptr)
                            onCloseClicked();
                    }
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync (
                        juce::AlertWindow::WarningIcon,
                        "Open Failed",
                        "Invalid file format.",
                        "OK"
                    );
                }
            }
        });
}

void GlobalMenuComponent::performExport()
{
    // Re-route to the existing popup export options
    juce::PopupMenu menu;
    menu.addItem (1, "Export Multitrack MIDI (.mid)");
    menu.addItem (2, "Export Offline Audio (.wav)");
    menu.addItem (3, "Export Sheet Music (.pdf)");

    menu.showMenuAsync (juce::PopupMenu::Options(), [this](int result)
    {
        if (result == 1)
        {
            performMidiExport (arrangement);
        }
        else if (result == 2)
        {
            // OfflineRenderThread is managed asynchronously
            auto* thread = new OfflineRenderThread (arrangement);
            thread->startThread();
        }
        else if (result == 3)
        {
            juce::PopupMenu pdfMenu;
            pdfMenu.addItem (1, "Chords Only");
            pdfMenu.addItem (2, "Full Sheet Music");
            pdfMenu.showMenuAsync (juce::PopupMenu::Options(), [this](int pdfResult)
            {
                if (pdfResult == 1)
                {
                    performSheetMusicExport (arrangement, false);
                }
                else if (pdfResult == 2)
                {
                    performSheetMusicExport (arrangement, true);
                }
            });
        }
    });
}

void GlobalMenuComponent::performAutoComposer()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Plain Pop");
    menu.addItem (2, "Justified Jazz");
    menu.addItem (3, "Cinematic Epic");
    menu.addItem (4, "Lo-Fi Chill");

    menu.showMenuAsync (juce::PopupMenu::Options(), [this](int result)
    {
        juce::String theme;
        if (result == 1) theme = "Plain Pop";
        else if (result == 2) theme = "Justified Jazz";
        else if (result == 3) theme = "Cinematic Epic";
        else if (result == 4) theme = "Lo-Fi Chill";

        if (theme.isNotEmpty())
        {
            // Start Auto-Composer Async
            auto* ai = new AiCoreManager();
            ai->generateFullTrackAsync (theme, [this, ai](const AiCoreManager::AutoComposerTrack& track)
            {
                // Update central arrangement
                arrangement.songName = track.name;
                arrangement.setTempo (track.bpm);
                arrangement.activeKey = track.key;

                auto& chords = arrangement.getChords();
                chords.clear();
                for (auto& c : track.chords)
                {
                    ChordBlock cb;
                    cb.root = c.root;
                    cb.quality = c.quality;
                    cb.inversion = c.inversion;
                    cb.durationBeats = c.durationBeats;
                    cb.durationText = c.durationText;
                    cb.octave = c.octave;

                    auto* def = ChordDatabase::getInstance().getChord (cb.root, cb.quality, cb.inversion);
                    if (def != nullptr)
                        cb.name = def->name;
                    else
                        cb.name = cb.root + " " + cb.quality;

                    cb.colour = juce::Colour (0xff0f172a);
                    cb.isSelected = false;
                    chords.add (cb);
                }
                if (! chords.isEmpty())
                    chords.getReference(0).isSelected = true;

                // Update tracks
                for (int i = 0; i < 13; ++i)
                {
                    arrangement.trackLanes[i].enabled = track.lanes[i].enabled;
                    arrangement.trackLanes[i].gmProgramNumber = track.lanes[i].gmProgramNumber;
                    arrangement.trackLanes[i].patternId = track.lanes[i].patternId;
                }

                arrangement.sendProgressionToAudioThread();
                arrangement.notifyChanges();

                juce::AlertWindow::showMessageBoxAsync (
                    juce::AlertWindow::InfoIcon,
                    "Auto-Composer Successful",
                    "Generated theme-based track:\n" + arrangement.songName,
                    "OK"
                );

                delete ai; // clean up temporary manager
                if (onCloseClicked != nullptr)
                    onCloseClicked();
            });
        }
    });
}

void GlobalMenuComponent::performHelp()
{
    if (onHelpClicked != nullptr)
        onHelpClicked();
}
