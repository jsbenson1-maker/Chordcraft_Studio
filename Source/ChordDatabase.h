#pragma once

#include <JuceHeader.h>
#include <unordered_map>
#include <vector>
#include <string>

struct ChordDefinition
{
    juce::String id;
    juce::String name;
    juce::String root;
    juce::String quality;
    int inversion = 0;
    int rootMidiOffset = 0;
    std::vector<int> intervals;
};

class ChordDatabase
{
public:
    ChordDatabase();
    ~ChordDatabase() = default;

    static ChordDatabase& getInstance();

    // Query a chord definition by root, quality, and inversion (O(1) lookup)
    const ChordDefinition* getChord (const juce::String& root, const juce::String& quality, int inversion) const;
    
    // Query a chord definition by explicit ID (e.g. "C_Maj_i0")
    const ChordDefinition* getChordById (const std::string& id) const;

    const juce::StringArray& getRoots() const { return roots; }
    const juce::StringArray& getQualities() const { return qualities; }
    
    int getNumChords() const { return static_cast<int> (chordMap.size()); }

private:
    void loadDatabase();

    std::unordered_map<std::string, ChordDefinition> chordMap;
    juce::StringArray roots;
    juce::StringArray qualities;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordDatabase)
};
