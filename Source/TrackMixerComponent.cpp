#include "TrackMixerComponent.h"
#include "ThemeManager.h"
#include "PatternDatabase.h"

//==============================================================================
TrackMixerComponent::TrackMixerComponent (ChordArrangement& arr)
    : arrangement (arr), listContent (*this)
{
    setWantsKeyboardFocus (true);
    arrangement.addChangeListener (this);

    addAndMakeVisible (viewport);
    viewport.setScrollBarsShown (true, false);
    viewport.setViewedComponent (&listContent, false);

    addAndMakeVisible (closeButton);
    closeButton.setButtonText ("CLOSE MIXER");
    closeButton.onClick = [this] {
        ThemeManager::triggerHapticImpact();
        if (onCloseClicked != nullptr)
            onCloseClicked();
    };

    listContent.updateTracks();
}

TrackMixerComponent::~TrackMixerComponent()
{
    arrangement.removeChangeListener (this);
}

void TrackMixerComponent::paint (juce::Graphics& g)
{
    // Draw background panel (Dark Purple/Navy)
    g.setColour (juce::Colour (0xff1e1e2e));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    // Glowing border
    g.setColour (ThemeManager::getSystemAccentColor());
    g.drawRoundedRectangle (getLocalBounds().toFloat(), 8.0f, 2.0f);

    // Header title
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.drawText ("TRACK MIXER & SEQUENCER", 0, 8, getWidth(), 35, juce::Justification::centred);
}

void TrackMixerComponent::resized()
{
    auto area = getLocalBounds().reduced (8);
    
    // Top title area space
    area.removeFromTop (45);

    // Close button at bottom
    int btnH = 36;
    closeButton.setBounds (area.removeFromBottom (btnH + 8).reduced (40, 4));

    // Viewport fills the rest
    viewport.setBounds (area);
    listContent.updateTracks();
}

void TrackMixerComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    listContent.updateTracks();
    repaint();
}

void TrackMixerComponent::mouseDrag (const juce::MouseEvent& event)
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

bool TrackMixerComponent::keyPressed (const juce::KeyPress& key)
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

void TrackMixerComponent::visibilityChanged()
{
    if (isVisible())
    {
        grabKeyboardFocus();
        hasSwipedDismiss = false;
    }
}

//==============================================================================
// MixerListContent Implementation
//==============================================================================
TrackMixerComponent::MixerListContent::MixerListContent (TrackMixerComponent& o)
    : owner (o)
{
    addAndMakeVisible (addMelodicButton);
    addMelodicButton.setButtonText ("+ Add Melodic");
    addMelodicButton.onClick = [this] {
        owner.arrangement.saveActiveSection();
        TrackSettings ts;
        ts.enabled = true;
        ts.gmProgramNumber = 0;
        ts.patternId = "";
        ts.volume = 0.6f;
        ts.isDrums = false;
        owner.arrangement.trackLanes.push_back (ts);
        owner.arrangement.sendProgressionToAudioThread();
        owner.arrangement.notifyChanges();
    };

    addAndMakeVisible (addDrumButton);
    addDrumButton.setButtonText ("+ Add Drums");
    addDrumButton.onClick = [this] {
        owner.arrangement.saveActiveSection();
        TrackSettings ts;
        ts.enabled = true;
        ts.gmProgramNumber = 0;
        ts.patternId = "drums_rock_basic_+_8th_hats_742";
        ts.volume = 0.6f;
        ts.isDrums = true;
        owner.arrangement.trackLanes.push_back (ts);
        owner.arrangement.sendProgressionToAudioThread();
        owner.arrangement.notifyChanges();
    };
}

void TrackMixerComponent::MixerListContent::resized()
{
    int y = 4;
    int rowH = 68;
    int w = getWidth() - 12;

    for (auto* row : rows)
    {
        row->setBounds (4, y, w, rowH - 4);
        y += rowH;
    }

    int btnW = (w - 12) / 2;
    addMelodicButton.setBounds (4, y + 8, btnW, 32);
    addDrumButton.setBounds (4 + btnW + 8, y + 8, btnW, 32);
}

void TrackMixerComponent::MixerListContent::updateTracks()
{
    rows.clear();
    int numTracks = (int) owner.arrangement.trackLanes.size();

    for (int i = 0; i < numTracks; ++i)
    {
        auto* row = rows.add (new TrackRow (*this, i));
        addAndMakeVisible (row);
    }

    int totalH = numTracks * 68 + 50;
    setSize (owner.viewport.getWidth() - 16, totalH);
    resized();
}

//==============================================================================
// TrackRow Implementation
//==============================================================================
TrackMixerComponent::MixerListContent::TrackRow::TrackRow (MixerListContent& o, int index)
    : owner (o), idx (index)
{
    auto& lane = owner.owner.arrangement.trackLanes[idx];

    addAndMakeVisible (enableToggle);
    enableToggle.setToggleState (lane.enabled, juce::dontSendNotification);
    enableToggle.onClick = [this] {
        ThemeManager::triggerHapticRatchet();
        owner.owner.arrangement.trackLanes[idx].enabled = enableToggle.getToggleState();
        owner.owner.arrangement.sendProgressionToAudioThread();
    };

    addAndMakeVisible (trackLabel);
    int channelNum = 1;
    if (lane.isDrums)
    {
        channelNum = 10;
    }
    else
    {
        int melodicCount = 0;
        for (int i = 0; i <= idx; ++i)
        {
            if (! owner.owner.arrangement.trackLanes[i].isDrums)
                melodicCount++;
        }
        int mCount = 0;
        for (int ch = 0; ch < 16; ++ch)
        {
            if (ch == 9) continue;
            mCount++;
            if (mCount == melodicCount)
            {
                channelNum = ch + 1;
                break;
            }
        }
    }
    juce::String labelText = lane.isDrums ? "Drums (ch10)" : "Track " + juce::String (idx + 1) + " (ch" + juce::String (channelNum) + ")";
    trackLabel.setText (labelText, juce::dontSendNotification);
    trackLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    trackLabel.setFont (juce::Font (12.0f, juce::Font::bold));

    addAndMakeVisible (instrumentButton);
    instrumentButton.setButtonText (lane.isDrums ? "Drums" : getInstrumentName (lane.gmProgramNumber));
    instrumentButton.setEnabled (! lane.isDrums);
    instrumentButton.onClick = [this] { chooseInstrument(); };

    addAndMakeVisible (patternButton);
    juce::String patName = "None (Chords)";
    if (! lane.patternId.isEmpty())
    {
        if (auto* pat = PatternDatabase::getInstance().getPatternById (lane.patternId.toStdString()))
            patName = pat->name;
        else
            patName = lane.patternId;
    }
    patternButton.setButtonText (patName);
    patternButton.onClick = [this] { choosePattern(); };

    addAndMakeVisible (volumeSlider);
    volumeSlider.setRange (0.0, 1.0, 0.01);
    volumeSlider.setValue (lane.volume, juce::dontSendNotification);
    volumeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.addListener (this);

    addAndMakeVisible (removeButton);
    removeButton.setButtonText ("Remove");
    removeButton.onClick = [this] {
        ThemeManager::triggerHapticImpact();
        owner.owner.arrangement.saveActiveSection();
        if (owner.owner.arrangement.trackLanes.size() > 1)
        {
            owner.owner.arrangement.trackLanes.erase (owner.owner.arrangement.trackLanes.begin() + idx);
            owner.owner.arrangement.sendProgressionToAudioThread();
            owner.owner.arrangement.notifyChanges();
        }
    };
}

void TrackMixerComponent::MixerListContent::TrackRow::paint (juce::Graphics& g)
{
    auto& lane = owner.owner.arrangement.trackLanes[idx];
    juce::Colour rowBg = (idx % 2 == 0) ? juce::Colour (0xff161622) : juce::Colour (0xff1b1b28);
    g.fillAll (rowBg);

    g.setColour (lane.enabled ? ThemeManager::getSystemAccentColor().withAlpha (0.3f) : juce::Colour (0xff334155));
    g.drawRect (getLocalBounds(), 1);
}

void TrackMixerComponent::MixerListContent::TrackRow::resized()
{
    auto bounds = getLocalBounds().reduced (4);
    auto topRow = bounds.removeFromTop (bounds.getHeight() / 2);
    auto bottomRow = bounds;

    // Top: [Toggle] [Label] ........ [Instrument] [Remove]
    enableToggle.setBounds (topRow.removeFromLeft (24));
    trackLabel.setBounds (topRow.removeFromLeft (80));
    removeButton.setBounds (topRow.removeFromRight (60));
    instrumentButton.setBounds (topRow.removeFromRight (140).reduced (2));

    // Bottom: [Pattern] ............ [Volume Slider]
    patternButton.setBounds (bottomRow.removeFromLeft (120).reduced (2));
    volumeSlider.setBounds (bottomRow.reduced (4, 2));
}

void TrackMixerComponent::MixerListContent::TrackRow::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &volumeSlider)
    {
        float vol = (float) volumeSlider.getValue();
        owner.owner.arrangement.trackLanes[idx].volume = vol;
        
        // Push volume change directly to the audio thread
        if (auto* proc = owner.owner.arrangement.getAudioProcessor())
        {
            // Determine track channel
            int melodicCount = 0;
            int channel = 0;
            auto& lanes = owner.owner.arrangement.trackLanes;
            for (int i = 0; i <= idx; ++i)
            {
                if (lanes[i].isDrums)
                {
                    if (i == idx) channel = 9;
                }
                else
                {
                    if (melodicCount == 9) melodicCount++;
                    if (i == idx) channel = melodicCount;
                    melodicCount++;
                }
            }
            if (channel > 15) channel = 15;

            PlaybackInstruction volInst;
            volInst.type = PlaybackInstruction::SetChannelVolume;
            volInst.blockIndex = channel;
            volInst.tempo = vol;
            proc->queueInstruction (volInst);
        }
    }
}

void TrackMixerComponent::MixerListContent::TrackRow::chooseInstrument()
{
    juce::PopupMenu m;
    const char* categories[] = {
        "Piano", "Chromatic Percussion", "Organ", "Guitar", "Bass", "Strings",
        "Ensemble", "Brass", "Reed", "Pipe", "Synth Lead", "Synth Pad",
        "Synth FX", "Ethnic", "Percussive", "Sound Effects"
    };

    auto& lane = owner.owner.arrangement.trackLanes[idx];

    for (int catIdx = 0; catIdx < 16; ++catIdx)
    {
        juce::PopupMenu sub;
        for (int p = 0; p < 8; ++p)
        {
            int prog = catIdx * 8 + p;
            sub.addItem (prog + 1, getInstrumentName (prog), true, lane.gmProgramNumber == prog);
        }
        m.addSubMenu (categories[catIdx], sub);
    }

    m.showMenuAsync (juce::PopupMenu::Options(), [this](int result) {
        if (result > 0)
        {
            int newProgram = result - 1;
            owner.owner.arrangement.trackLanes[idx].gmProgramNumber = newProgram;

            // Clear incompatible patterns
            auto& lane = owner.owner.arrangement.trackLanes[idx];
            if (! lane.patternId.isEmpty())
            {
                auto* pat = PatternDatabase::getInstance().getPatternById (lane.patternId.toStdString());
                if (pat != nullptr)
                {
                    juce::String newFamily = getGMFamilyName (newProgram);
                    bool isCompatible = false;
                    for (auto& fam : pat->compatibleFamilies)
                    {
                        if (fam == newFamily) { isCompatible = true; break; }
                    }
                    if (! isCompatible) lane.patternId = "";
                }
            }

            ThemeManager::triggerHapticImpact();
            owner.owner.arrangement.sendProgressionToAudioThread();
            owner.owner.arrangement.notifyChanges();
        }
    });
}

void TrackMixerComponent::MixerListContent::TrackRow::choosePattern()
{
    auto& lane = owner.owner.arrangement.trackLanes[idx];
    juce::PopupMenu m;
    m.addItem (1, "None (Chords)", true, lane.patternId.isEmpty());

    auto& db = PatternDatabase::getInstance();
    auto ids = db.getPatternIds();
    juce::String activeFamily = lane.isDrums ? "Drums" : getGMFamilyName (lane.gmProgramNumber);

    std::vector<const PatternDefinition*> compatiblePatterns;
    for (auto& id : ids)
    {
        auto* def = db.getPatternById (id);
        if (def != nullptr)
        {
            for (auto& fam : def->compatibleFamilies)
            {
                if (isFamilyCompatible (fam, activeFamily)) { compatiblePatterns.push_back (def); break; }
            }
        }
    }

    int itemIndex = 2;
    std::vector<std::string> validIds;
    validIds.push_back ("");

    juce::PopupMenu subMenu;
    for (auto* def : compatiblePatterns)
    {
        subMenu.addItem (itemIndex, def->name, true, lane.patternId == def->id);
        validIds.push_back (def->id.toStdString());
        itemIndex++;
    }
    m.addSubMenu (activeFamily + " Patterns", subMenu);

    m.showMenuAsync (juce::PopupMenu::Options(), [this, validIds](int result) {
        if (result > 0)
        {
            if (result == 1)
                owner.owner.arrangement.trackLanes[idx].patternId = "";
            else if (result - 1 < (int) validIds.size())
                owner.owner.arrangement.trackLanes[idx].patternId = validIds[result - 1];

            ThemeManager::triggerHapticImpact();
            owner.owner.arrangement.sendProgressionToAudioThread();
            owner.owner.arrangement.notifyChanges();
        }
    });
}

juce::String TrackMixerComponent::MixerListContent::TrackRow::getInstrumentName (int programNum) const
{
    static const char* gmInstruments[] = {
        "Piano", "Bright Piano", "Electric Grand", "Honky-tonk", "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavi",
        "Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
        "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
        "Nylon Guitar", "Steel Guitar", "Jazz Guitar", "Clean Guitar", "Muted Guitar", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics",
        "Acoustic Bass", "Finger Bass", "Pick Bass", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
        "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
        "String Ensemble 1", "String Ensemble 2", "SynthStrings 1", "SynthStrings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
        "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "SynthBrass 1", "SynthBrass 2",
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
        "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
        "Square Lead", "Sawtooth Lead", "Calliope Lead", "Chiff Lead", "Charang Lead", "Voice Lead", "Fifths Lead", "Bass+Lead",
        "New Age Pad", "Warm Pad", "Polysynth Pad", "Choir Pad", "Bowed Pad", "Metallic Pad", "Halo Pad", "Sweep Pad",
        "Rain FX", "Soundtrack FX", "Crystal FX", "Atmosphere FX", "Brightness FX", "Goblins FX", "Echoes FX", "Sci-Fi FX",
        "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bagpipe", "Fiddle", "Shanai",
        "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
        "Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot"
    };

    if (programNum >= 0 && programNum < 128) return gmInstruments[programNum];
    return "Piano";
}

juce::String TrackMixerComponent::MixerListContent::TrackRow::getGMFamilyName (int programNum) const
{
    int familyIndex = programNum / 8;
    switch (familyIndex)
    {
        case 0:  return "Piano";
        case 1:  return "Chromatic Percussion";
        case 2:  return "Organ";
        case 3:  return "Guitar";
        case 4:  return "Bass";
        case 5:  return "Strings";
        case 6:  return "Strings";
        case 7:  return "Brass";
        case 8:  return "Reed";
        case 9:  return "Pipe";
        case 10: return "Synth Lead";
        case 11: return "Synth Pad";
        case 12: return "Synth Pad";
        case 13: return "Ethnic";
        case 14: return "Percussive";
        case 15: return "Sound Effects";
        default: return "Piano";
    }
}
