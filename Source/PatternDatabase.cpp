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

bool isFamilyCompatible (const juce::String& fam, const juce::String& activeFamily)
{
    if (fam.equalsIgnoreCase (activeFamily))
        return true;
        
    if (activeFamily.equalsIgnoreCase ("Bass") && fam.containsIgnoreCase ("Bass"))
        return true;
        
    if (activeFamily.equalsIgnoreCase ("Strings") || activeFamily.equalsIgnoreCase ("Ensemble"))
    {
        juce::StringArray str = { "Violin", "Viola", "Cello", "Contrabass", "Strings", "Double Bass" };
        for (auto& s : str)
            if (fam.equalsIgnoreCase (s)) return true;
    }
    
    if (activeFamily.equalsIgnoreCase ("Reed"))
    {
        juce::StringArray reed = { "Oboe", "Clarinet", "Bassoon", "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "English Horn" };
        for (auto& r : reed)
            if (fam.equalsIgnoreCase (r)) return true;
    }
    
    if (activeFamily.equalsIgnoreCase ("Pipe"))
    {
        juce::StringArray pipe = { "Flute", "Piccolo", "Recorder", "Pan Flute", "Ocarina", "Shakuhachi", "Whistle" };
        for (auto& p : pipe)
            if (fam.equalsIgnoreCase (p)) return true;
    }
    
    if (activeFamily.equalsIgnoreCase ("Brass"))
    {
        juce::StringArray brass = { "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section" };
        for (auto& b : brass)
            if (fam.equalsIgnoreCase (b)) return true;
    }
    
    if ((activeFamily.equalsIgnoreCase ("Synth Lead") || activeFamily.equalsIgnoreCase ("Synth Effects") || activeFamily.equalsIgnoreCase ("Synth FX"))
        && (fam.containsIgnoreCase ("Lead") || fam.containsIgnoreCase ("Synth Lead")))
        return true;
        
    if ((activeFamily.equalsIgnoreCase ("Synth Pad") || activeFamily.equalsIgnoreCase ("Synth Effects") || activeFamily.equalsIgnoreCase ("Synth FX"))
        && (fam.containsIgnoreCase ("Pad") || fam.containsIgnoreCase ("Synth Pad")))
        return true;
        
    return false;
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
                if (isFamilyCompatible (fam, instrument))
                {
                    result.add (def->id);
                    break;
                }
            }
        }
    }
    return result;
}
