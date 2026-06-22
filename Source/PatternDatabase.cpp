#include "PatternDatabase.h"

PatternDatabase& PatternDatabase::getInstance()
{
    static PatternDatabase instance;
    return instance;
}

PatternDatabase::PatternDatabase()
{
    loadDatabase();
}

void PatternDatabase::loadDatabase()
{
    // Create a String from the embedded binary resource
    juce::String jsonText = juce::String::createStringFromData (BinaryData::patterns_master_json, BinaryData::patterns_master_jsonSize);
    auto parsed = juce::JSON::parse (jsonText);
    
    if (auto* array = parsed.getArray())
    {
        for (auto& var : *array)
        {
            if (auto* obj = var.getDynamicObject())
            {
                PatternDefinition def;
                def.id = obj->getProperty ("id").toString();
                def.name = obj->getProperty ("name").toString();
                def.category = obj->getProperty ("category").toString();
                
                // Read compatible families array
                auto compFamVar = obj->getProperty ("compatibleFamilies");
                if (auto* compFamArray = compFamVar.getArray())
                {
                    for (auto& f : *compFamArray)
                        def.compatibleFamilies.push_back (f.toString());
                }
                
                // Read notes from "events" array
                auto notesVar = obj->getProperty ("events");
                if (auto* notesArray = notesVar.getArray())
                {
                    for (auto& val : *notesArray)
                    {
                        if (auto* noteObj = val.getDynamicObject())
                        {
                            PatternNote note;
                            note.tick = static_cast<int> (noteObj->getProperty ("tick"));
                            note.velocity = static_cast<int> (noteObj->getProperty ("velocity"));
                            
                            if (noteObj->hasProperty ("duration"))
                                note.duration = static_cast<int> (noteObj->getProperty ("duration"));
                            else
                                note.duration = 240; // Default duration for drum hits
                            
                            if (noteObj->hasProperty ("degree"))
                                note.degree = static_cast<int> (noteObj->getProperty ("degree"));
                            else
                                note.degree = std::nullopt;
                                
                            if (noteObj->hasProperty ("drumMidi"))
                                note.drumMidi = static_cast<int> (noteObj->getProperty ("drumMidi"));
                            else
                                note.drumMidi = std::nullopt;
                                
                            def.notes.push_back (note);
                        }
                    }
                }
                
                std::string stdId = def.id.toStdString();
                patternMap[stdId] = def;
                patternIds.push_back (stdId);
            }
        }
    }
    
    DBG ("PatternDatabase: Loaded " + juce::String (patternMap.size()) + " patterns.");
}

const PatternDefinition* PatternDatabase::getPatternById (const std::string& id) const
{
    auto it = patternMap.find (id);
    if (it != patternMap.end())
        return &(it->second);
    return nullptr;
}

juce::StringArray PatternDatabase::getPatternIdsForInstrument (const juce::String& instrument) const
{
    juce::StringArray result;
    for (auto& pid : patternIds)
    {
        auto* def = getPatternById (pid);
        if (def != nullptr)
        {
            for (auto& fam : def->compatibleFamilies)
            {
                if (fam.equalsIgnoreCase (instrument))
                {
                    result.add (def->id);
                    break;
                }
            }
        }
    }
    return result;
}
