#pragma once

#include <JuceHeader.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>

struct PatternNote
{
    int tick = 0;
    int duration = 0;
    int velocity = 100;
    std::optional<int> degree;       // For melodic instruments
    std::optional<int> drumMidi;     // For drums
};

struct PatternDefinition
{
    juce::String id;
    juce::String name;
    juce::String category;   // Group/genre category of the pattern
    juce::String instrument; // General MIDI instrument category/name
    std::vector<PatternNote> notes;
    std::vector<juce::String> compatibleFamilies;
};

bool isFamilyCompatible (const juce::String& fam, const juce::String& activeFamily);

class PatternDatabase
{
public:
    PatternDatabase();
    ~PatternDatabase() = default;

    static PatternDatabase& getInstance();

    // Query a pattern definition by explicit ID
    const PatternDefinition* getPatternById (const std::string& id) const;

    const std::vector<std::string>& getPatternIds() const { return patternIds; }
    
    // Get all pattern IDs associated with a specific instrument type
    juce::StringArray getPatternIdsForInstrument (const juce::String& instrument) const;

private:
    void loadDatabase();

    std::unordered_map<std::string, PatternDefinition> patternMap;
    std::vector<std::string> patternIds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatternDatabase)
};
