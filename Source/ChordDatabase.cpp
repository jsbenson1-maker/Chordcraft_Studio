#include "ChordDatabase.h"

ChordDatabase& ChordDatabase::getInstance()
{
    static ChordDatabase instance;
    return instance;
}

ChordDatabase::ChordDatabase()
{
    loadDatabase();
}

void ChordDatabase::loadDatabase()
{
    // Create a String from the embedded binary resource
    juce::String jsonText = juce::String::createStringFromData (BinaryData::chords_json, BinaryData::chords_jsonSize);
    auto parsed = juce::JSON::parse (jsonText);
    
    if (auto* array = parsed.getArray())
    {
        for (auto& var : *array)
        {
            if (auto* obj = var.getDynamicObject())
            {
                ChordDefinition def;
                def.id = obj->getProperty ("id").toString();
                def.name = obj->getProperty ("name").toString();
                def.root = obj->getProperty ("root").toString();
                def.quality = obj->getProperty ("quality").toString();
                def.inversion = static_cast<int> (obj->getProperty ("inversion"));
                def.rootMidiOffset = static_cast<int> (obj->getProperty ("rootMidiOffset"));
                
                auto intervalsVar = obj->getProperty ("intervals");
                if (auto* intervalsArray = intervalsVar.getArray())
                {
                    for (auto& val : *intervalsArray)
                    {
                        def.intervals.push_back (static_cast<int> (val));
                    }
                }
                
                // Construct standard key: e.g., "C_Maj_i0"
                std::string key = def.id.toStdString();
                chordMap[key] = def;
                
                // Keep track of unique roots and qualities for lookup reference
                if (! roots.contains (def.root))
                    roots.add (def.root);
                if (! qualities.contains (def.quality))
                    qualities.add (def.quality);
            }
        }
    }
    
    DBG ("ChordDatabase: Loaded " + juce::String (getNumChords()) + " chords.");
}

const ChordDefinition* ChordDatabase::getChord (const juce::String& root, const juce::String& quality, int inversion) const
{
    // Generates key of format root_quality_i{inversion} matching JSON IDs (e.g. "C_Maj_i0")
    std::string key = (root + "_" + quality + "_i" + juce::String (inversion)).toStdString();
    return getChordById (key);
}

const ChordDefinition* ChordDatabase::getChordById (const std::string& id) const
{
    auto it = chordMap.find (id);
    if (it != chordMap.end())
        return &(it->second);
    return nullptr;
}
