#pragma once
#include <JuceHeader.h>
#include "ChordcraftAudioProcessor.h"
#include "ChordDatabase.h"

struct ChordBlock
{
    juce::String name;
    juce::String root;
    juce::String quality;
    int inversion = 0;
    int durationBeats = 4; 
    juce::String durationText = "4/4";
    juce::Colour colour;
    bool isSelected = false;
    juce::Rectangle<int> bounds; 
    juce::Array<int> midiNotes; // Resolved MIDI notes for audio playback
    int octave = 1; // 0 = Low, 1 = Mid, 2 = High (default 1)
    juce::String customBassNote;
};

static inline juce::Array<int> resolveChordMidiNotes (const ChordBlock& cb)
{
    juce::Array<int> chordMidiNotes;
    int targetInversion = cb.customBassNote.isNotEmpty() ? 0 : cb.inversion;
    std::string keyId = (cb.root + "_" + cb.quality + "_i" + juce::String (targetInversion)).toStdString();
    auto* def = ChordDatabase::getInstance().getChordById (keyId);
    
    int rootMidi = 60;
    if (cb.octave == 0) rootMidi = 48;
    else if (cb.octave == 2) rootMidi = 72;
    
    if (def != nullptr)
    {
        if (def->root == "Db") rootMidi += 1;
        else if (def->root == "D") rootMidi += 2;
        else if (def->root == "Eb") rootMidi += 3;
        else if (def->root == "E") rootMidi += 4;
        else if (def->root == "F") rootMidi += 5;
        else if (def->root == "Gb") rootMidi += 6;
        else if (def->root == "G") rootMidi += 7;
        else if (def->root == "Ab") rootMidi += 8;
        else if (def->root == "A") rootMidi += 9;
        else if (def->root == "Bb") rootMidi += 10;
        else if (def->root == "B") rootMidi += 11;
        
        if (cb.customBassNote.isNotEmpty())
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
            int rootPitchClass = getPitchClass (cb.root);
            int bassPitchClass = getPitchClass (cb.customBassNote);
            int diff = (bassPitchClass - rootPitchClass + 12) % 12;
            int bassMidi = rootMidi - 12 + diff;
            chordMidiNotes.add (bassMidi);
        }
        
        int numNotes = juce::jmin (8, (int) def->intervals.size());
        for (int n = 0; n < numNotes; ++n)
            chordMidiNotes.add (rootMidi + def->intervals[n] + def->rootMidiOffset);
    }
    else
    {
        if (cb.customBassNote.isNotEmpty())
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
            int rootPitchClass = getPitchClass (cb.root);
            int bassPitchClass = getPitchClass (cb.customBassNote);
            int diff = (bassPitchClass - rootPitchClass + 12) % 12;
            int bassMidi = rootMidi - 12 + diff;
            chordMidiNotes.add (bassMidi);
        }
        chordMidiNotes.add (rootMidi);
        chordMidiNotes.add (rootMidi + 4);
        chordMidiNotes.add (rootMidi + 7);
    }
    return chordMidiNotes;
}

struct TrackSettings
{
    bool enabled = true;
    int gmProgramNumber = 0;     // 0-127 (default 0: Acoustic Grand Piano)
    juce::String patternId = ""; // Empty string represents no pattern
    float volume = 0.6f;         // Track volume range [0.0f, 1.0f], default 0.6f
    bool isDrums = false;        // Flag to identify drum tracks
};

struct SongSection
{
    std::string sectionName;
    int loopCount = 1;
    std::vector<ChordBlock> blocks;
    double bpm = 120.0;
    juce::String currentKey = "C Maj";
    std::vector<TrackSettings> tracks;
};

class ChordArrangement : public juce::ChangeBroadcaster, public juce::Timer
{
public:
    ChordArrangement()
    {
        // Initial default section
        SongSection defaultSection;
        defaultSection.sectionName = "Section 1";
        defaultSection.loopCount = 1;
        defaultSection.bpm = 120.0;
        defaultSection.currentKey = "C Maj";

        // Initial default blocks
        defaultSection.blocks.push_back ({ "C Maj",  "C", "Maj",  0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
        defaultSection.blocks.push_back ({ "G Min",  "G", "Min",  0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
        defaultSection.blocks.push_back ({ "A Min",  "A", "Min",  0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
        defaultSection.blocks.push_back ({ "F Maj7", "F", "Maj7", 0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });

        // Initialize 13 lanes (12 melodic + 1 drum)
        for (int i = 0; i < 12; ++i)
        {
            TrackSettings ts;
            ts.enabled = true;
            ts.gmProgramNumber = 0;
            ts.patternId = "";
            ts.volume = 0.6f;
            ts.isDrums = false;
            defaultSection.tracks.push_back (ts);
        }
        
        TrackSettings drumTs;
        drumTs.enabled = true;
        drumTs.gmProgramNumber = 0;
        drumTs.patternId = "drums_rock_basic_+_8th_hats_742";
        drumTs.volume = 0.6f;
        drumTs.isDrums = true;
        defaultSection.tracks.push_back (drumTs);

        sections.push_back (defaultSection);

        // Load into active public members
        chords.clear();
        for (auto& cb : defaultSection.blocks)
            chords.add (cb);
        trackLanes = defaultSection.tracks;
        bpm = 120.0f;
        activeKey = "C Maj";
        
        startTimerHz (30);
    }
    
    ~ChordArrangement() override
    {
        stopTimer();
    }
    
    // Core engine trackers
    bool isMainSongPlaying = false;
    bool wasPreviewing = false;

    void timerCallback() override
    {
        if (audioProcessor != nullptr)
        {
            bool enginePlaying = audioProcessor->isEnginePlaying();
            bool isCurrentlyPreviewing = (!isMainSongPlaying && enginePlaying);
            
            // FIX 1: Auto-restore main song when preview finishes naturally
            if (wasPreviewing && !enginePlaying)
            {
                wasPreviewing = false;
                sendProgressionToAudioThread();
                return; 
            }

            wasPreviewing = isCurrentlyPreviewing;

            // FIX 2: Only track visual sections if the MAIN song is playing (Ignores Preview's track 0!)
            if (isMainSongPlaying && enginePlaying)
            {
                int currentPbIdx = audioProcessor->playingSectionIndexAtomic.load();
                if (currentPbIdx != lastPlaybackSectionIndex)
                {
                    lastPlaybackSectionIndex = currentPbIdx;
                    if (currentPbIdx >= 0 && currentPbIdx < (int)playbackToSongSectionMap.size())
                    {
                        int songSecIdx = playbackToSongSectionMap[currentPbIdx];
                        if (songSecIdx != activeSectionIndex)
                        {
                            activeSectionIndex = songSecIdx;
                            
                            // Silently load visual data without triggering an audio rebuild
                            auto& sec = sections[activeSectionIndex];
                            chords.clear();
                            for (auto& cb : sec.blocks) chords.add (cb);
                            trackLanes = sec.tracks;
                            bpm = (float) sec.bpm;
                            activeKey = sec.currentKey;
                            
                            notifyChanges();
                        }
                    }
                }
            }
        }
    }

    // Bind the audio processor to enable UI control of the audio thread
    void setAudioProcessor (ChordcraftAudioProcessor* processor)
    {
        audioProcessor = processor;
        if (audioProcessor != nullptr)
        {
            sendProgressionToAudioThread();
            setTempo (bpm);
        }
    }

    // Active public members kept in sync for backward compatibility
    juce::Array<ChordBlock> chords;
    std::vector<TrackSettings> trackLanes;
    float bpm = 120.0f;
    juce::String activeKey = "C Maj";
    juce::String songName = "Untitled";

    // Section management
    std::vector<SongSection> sections;
    int activeSectionIndex = 0;
    
    int lastPlaybackSectionIndex = -1;
    std::vector<int> playbackToSongSectionMap;

    void saveActiveSection()
    {
        if (activeSectionIndex >= 0 && activeSectionIndex < (int) sections.size())
        {
            auto& sec = sections[activeSectionIndex];
            sec.blocks.clear();
            for (int i = 0; i < chords.size(); ++i)
                sec.blocks.push_back (chords[i]);
            sec.tracks = trackLanes;
            sec.bpm = bpm;
            sec.currentKey = activeKey;
        }
    }

    void loadActiveSection (int index)
    {
        saveActiveSection();
        
        if (index >= 0 && index < (int) sections.size())
        {
            activeSectionIndex = index;
            auto& sec = sections[activeSectionIndex];
            
            chords.clear();
            for (auto& cb : sec.blocks)
                chords.add (cb);
                
            trackLanes = sec.tracks;
            bpm = (float) sec.bpm;
            activeKey = sec.currentKey;
            
            sendProgressionToAudioThread();
            notifyChanges();
        }
    }

    void addNewSection()
    {
        saveActiveSection();
        int activeIdx = activeSectionIndex;
        auto& parentSec = sections[activeIdx];
        
        SongSection newSec;
        newSec.sectionName = "Section " + juce::String (sections.size() + 1).toStdString();
        newSec.loopCount = 1;
        newSec.bpm = parentSec.bpm;
        newSec.currentKey = parentSec.currentKey;
        newSec.tracks = parentSec.tracks; // Copy track settings
        
        // Reset blocks to default C Maj 4/4
        newSec.blocks.push_back ({ "C Maj", "C", "Maj", 0, 4, "4/4", juce::Colour (0xff0f172a), false, {}, {}, 1 });
        
        sections.push_back (newSec);
        activeSectionIndex = (int) sections.size() - 1;
        
        // Load new section
        chords.clear();
        for (auto& cb : newSec.blocks)
            chords.add (cb);
            
        trackLanes = newSec.tracks;
        bpm = (float) newSec.bpm;
        activeKey = newSec.currentKey;
        
        sendProgressionToAudioThread();
        notifyChanges();
    }

    void deleteSection (int index)
    {
        if (sections.size() <= 1) return; // Cannot delete last section
        
        if (index >= 0 && index < (int) sections.size())
        {
            sections.erase (sections.begin() + index);
            if (activeSectionIndex >= (int) sections.size())
                activeSectionIndex = (int) sections.size() - 1;
                
            // Reload active section
            auto& sec = sections[activeSectionIndex];
            chords.clear();
            for (auto& cb : sec.blocks)
                chords.add (cb);
                
            trackLanes = sec.tracks;
            bpm = (float) sec.bpm;
            activeKey = sec.currentKey;
            
            sendProgressionToAudioThread();
            notifyChanges();
        }
    }

    void resetProgression()
    {
        sections.clear();
        
        SongSection defaultSection;
        defaultSection.sectionName = "Section 1";
        defaultSection.loopCount = 1;
        defaultSection.bpm = 120.0;
        defaultSection.currentKey = "C Maj";
        defaultSection.blocks.push_back ({ "C Maj", "C", "Maj", 0, 4, "4/4", juce::Colour (0xff0f172a), true, {}, {}, 1 });

        for (int i = 0; i < 12; ++i)
        {
            TrackSettings ts;
            ts.enabled = true;
            ts.gmProgramNumber = 0;
            ts.patternId = "";
            ts.volume = 0.6f;
            ts.isDrums = false;
            defaultSection.tracks.push_back (ts);
        }
        TrackSettings drumTs;
        drumTs.enabled = true;
        drumTs.gmProgramNumber = 0;
        drumTs.patternId = "drums_rock_basic_+_8th_hats_742";
        drumTs.volume = 0.6f;
        drumTs.isDrums = true;
        defaultSection.tracks.push_back (drumTs);
        
        sections.push_back (defaultSection);
        activeSectionIndex = 0;
        
        chords.clear();
        for (auto& cb : defaultSection.blocks)
            chords.add (cb);
            
        trackLanes = defaultSection.tracks;
        bpm = 120.0f;
        activeKey = "C Maj";
        songName = "Untitled";
        
        sendProgressionToAudioThread();
        notifyChanges();
    }

    int getPlayheadTick() const
    {
        if (audioProcessor != nullptr)
            return audioProcessor->getPlayheadTick();
        return 0;
    }

    void auditionNoteOn (int midiNote, int velocity)
    {
        if (audioProcessor != nullptr)
            audioProcessor->auditionNoteOn (midiNote, velocity);
    }

    void auditionNoteOff (int midiNote)
    {
        if (audioProcessor != nullptr)
            audioProcessor->auditionNoteOff (midiNote);
    }

    void clearAudition()
    {
        if (audioProcessor != nullptr)
            audioProcessor->clearAudition();
    }

    void setPlayState (bool play)
    {
        isMainSongPlaying = play;
        if (audioProcessor != nullptr)
        {
            if (play)
            {
                wasPreviewing = false; // Reset preview flag just in case
                sendProgressionToAudioThread();
            }
            else
            {
                lastPlaybackSectionIndex = -1; // Reset tracker so it flips correctly on next play
            }
                
            audioProcessor->setPlayState (play);
        }
    }

    bool isPlaying() const
    {
        // Enforce the strict UI binding to the Main Song state
        return isMainSongPlaying; 
    }

    void setTempo (float newBpm, bool applyGlobally = false)
    {
        bpm = newBpm;
        if (applyGlobally)
        {
            for (auto& sec : sections)
                sec.bpm = newBpm;
        }
        else
        {
            if (activeSectionIndex >= 0 && activeSectionIndex < (int) sections.size())
                sections[activeSectionIndex].bpm = newBpm;
        }

        sendProgressionToAudioThread();
        if (audioProcessor != nullptr)
            audioProcessor->setTempo (newBpm);
        notifyChanges();
    }

    float getBpm() const { return bpm; }

    void sendProgressionToAudioThread()
    {
        saveActiveSection();
        
        // Map audio engine sequential loops back to UI visual tabs
        playbackToSongSectionMap.clear();
        for (int s = 0; s < (int) sections.size(); ++s)
        {
            auto& sec = sections[s];
            int secDurationTicks = 0;
            for (auto& cb : sec.blocks) secDurationTicks += cb.durationBeats * 960;
            if (secDurationTicks <= 0) continue;
            
            for (int loop = 0; loop < sec.loopCount; ++loop)
                playbackToSongSectionMap.push_back (s);
        }
        
        if (audioProcessor != nullptr)
        {
            audioProcessor->sendProgressionToAudioThread (sections, activeSectionIndex);
        }
    }

    // Global UI state for Clipboard Drawer
    bool isClipboardModeActive = false; 
    bool isLoopingEnabled = true;
    bool isChordPreviewEnabled = true;

    juce::Array<ChordBlock>& getChords() { return chords; }
    ChordcraftAudioProcessor* getAudioProcessor() const { return audioProcessor; }

    void notifyChanges()
    {
        sendChangeMessage(); // Notifies UI to repaint
    }

private:
    ChordcraftAudioProcessor* audioProcessor = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordArrangement)
};