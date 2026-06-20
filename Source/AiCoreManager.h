#pragma once

#include <JuceHeader.h>
#include <functional>
#include <string>

#if JUCE_ANDROID
#include <jni.h>
#endif

// Struct to hold suggestions categorized into three tiers
struct HarmonicAdvice
{
    juce::StringArray safe;         // 🟢 Diatonic / Safe
    juce::StringArray tension;      // 🟡 Modal / Borrowed / Tension
    juce::StringArray experimental; // 🔴 Chromatic / Experimental
};

class AiCoreManager : public juce::Thread
{
public:
    AiCoreManager()
        : juce::Thread ("AI Core Advice Inference Thread")
    {
    }

    ~AiCoreManager() override
    {
        stopThread (4000);
    }

    // Trigger AI inference in a background thread
    void triggerInferenceAsync (const juce::StringArray& chordHistory,
                                 const juce::String& activeKey,
                                 std::function<void(const HarmonicAdvice&)> callback)
    {
        // Cancel any running thread
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            stopThread (2000);
        }

        pendingHistory = chordHistory;
        pendingActiveKey = activeKey;
        pendingCallback = callback;

        startThread();
    }

    // Perform rule-based fallback calculation (Circle-of-Fifths / Diatonic relationships)
    static HarmonicAdvice getMusicTheoryFallback (const juce::StringArray& history, const juce::String& activeKey)
    {
        HarmonicAdvice advice;
        
        juce::String keyRoot = "C";
        bool isMajorKey = true;
        
        juce::StringArray keyTokens;
        keyTokens.addTokens (activeKey, " ", "");
        if (keyTokens.size() > 0)
            keyRoot = keyTokens[0];
        if (keyTokens.size() > 1 && keyTokens[1].equalsIgnoreCase ("Min"))
            isMajorKey = false;

        juce::StringArray roots = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
        int keyPitch = roots.indexOf (keyRoot);
        if (keyPitch == -1) keyPitch = 0;

        auto getRootAtOffset = [&](int semitones) {
            return roots[(keyPitch + semitones) % 12];
        };

        if (isMajorKey)
        {
            // Major scale degree suggestions
            advice.safe.add (getRootAtOffset (0) + " Maj");     // I
            advice.safe.add (getRootAtOffset (5) + " Maj7");    // IV
            advice.safe.add (getRootAtOffset (7) + " 7");       // V
            advice.safe.add (getRootAtOffset (9) + " Min7");    // vi
            advice.safe.add (getRootAtOffset (2) + " Min7");    // ii

            // Modal mixtures & secondary dominants (Yellow)
            advice.tension.add (getRootAtOffset (5) + " Min");    // iv
            advice.tension.add (getRootAtOffset (10) + " Maj7");  // bVII
            advice.tension.add (getRootAtOffset (8) + " Maj7");   // bVI
            advice.tension.add (getRootAtOffset (2) + " 7");      // V/V
            advice.tension.add (getRootAtOffset (4) + " 7");      // V/vi

            // Chromatic / Experimental (Red)
            advice.experimental.add (getRootAtOffset (1) + " Maj7");  // bII
            advice.experimental.add (getRootAtOffset (6) + " Maj");   // bV
            advice.experimental.add (getRootAtOffset (11) + " 7");    // V/iii
            advice.experimental.add (getRootAtOffset (3) + " Maj7");  // bIII
        }
        else
        {
            // Minor scale degree suggestions
            advice.safe.add (getRootAtOffset (0) + " Min");     // i
            advice.safe.add (getRootAtOffset (3) + " Maj7");    // III
            advice.safe.add (getRootAtOffset (5) + " Min7");    // iv
            advice.safe.add (getRootAtOffset (7) + " 7");       // V
            advice.safe.add (getRootAtOffset (8) + " Maj7");    // VI

            // Modal mixtures (Yellow)
            advice.tension.add (getRootAtOffset (5) + " Maj");    // IV (Dorian)
            advice.tension.add (getRootAtOffset (2) + " Min");    // ii
            advice.tension.add (getRootAtOffset (10) + " 7");     // VII
            advice.tension.add (getRootAtOffset (0) + " 7");      // Picardy/Tonic dominant

            // Chromatic / Experimental (Red)
            advice.experimental.add (getRootAtOffset (1) + " Maj7");  // bII
            advice.experimental.add (getRootAtOffset (6) + " Maj");   // bV
            advice.experimental.add (getRootAtOffset (11) + " Dim");  // vii°
        }

        return advice;
    }

    bool isModelAvailable() const
    {
#if JUCE_ANDROID
        return isModelAvailableAndroid();
#else
        return false;
#endif
    }

    void run() override
    {
        HarmonicAdvice result;

        // 1. Construct prompt
        juce::String progressionStr = pendingHistory.joinIntoString (", ");
        
        bool success = false;

#if JUCE_ANDROID
        if (isModelAvailableAndroid())
        {
            juce::String prompt = "You are a harmonic progression generator. "
                                  "The song is in the key of " + pendingActiveKey + ". "
                                  "The current progression is: [" + progressionStr + "]. "
                                  "Provide exactly 9 suggestions for the next chord. "
                                  "Return them formatted strictly as a JSON object with three string arrays: "
                                  "\"safe\", \"tension\", and \"experimental\". "
                                  "\"safe\" must be diatonic to " + pendingActiveKey + ". "
                                  "\"tension\" must be modal mixtures or secondary dominants. "
                                  "\"experimental\" must be chromatic/experimental. "
                                  "Example: "
                                  "{\"safe\":[\"F Maj\",\"G 7\",\"A Min\"],\"tension\":[\"F Min\",\"Ab Maj\",\"Bb Maj\"],\"experimental\":[\"Db Maj\",\"Gb Maj\",\"Eb 7\"]}";
            
            juce::String aiResponse = generateResponseAndroid (prompt);
            if (aiResponse.isNotEmpty())
            {
                parseAiResponse (aiResponse, result);
                success = (result.safe.size() > 0 || result.tension.size() > 0 || result.experimental.size() > 0);
            }
        }
#endif

        if (! success)
        {
            // Lightweight simulated delay for thread parity and natural UI flow
            juce::Thread::sleep (150);
            result = getMusicTheoryFallback (pendingHistory, pendingActiveKey);
        }

        if (! threadShouldExit())
        {
            auto callback = pendingCallback;
            juce::MessageManager::callAsync ([result, callback]() {
                callback (result);
            });
        }
    }

private:
    juce::StringArray pendingHistory;
    juce::String pendingActiveKey;
    std::function<void(const HarmonicAdvice&)> pendingCallback;

#if JUCE_ANDROID
    bool isModelAvailableAndroid() const
    {
        JNIEnv* env = juce::getEnv();
        if (env == nullptr) return false;

        jclass llmClass = env->FindClass ("com/google/android/aicore/llminference/LlmInference");
        if (llmClass == nullptr)
        {
            env->ExceptionClear();
            return false;
        }

        env->DeleteLocalRef (llmClass);
        return true;
    }

    juce::String generateResponseAndroid (const juce::String& prompt)
    {
        JNIEnv* env = juce::getEnv();
        if (env == nullptr) return {};

        auto activityLocalRef = juce::getCurrentActivity();
        jobject activity = activityLocalRef.get();
        if (activity == nullptr) return {};

        jclass builderClass = env->FindClass ("com/google/android/aicore/llminference/LlmInference$Builder");
        if (builderClass == nullptr)
        {
            env->ExceptionClear();
            return {};
        }

        jmethodID builderCtor = env->GetMethodID (builderClass, "<init>", "(Landroid/content/Context;)V");
        if (builderCtor == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (builderClass);
            return {};
        }

        jobject builderObj = env->NewObject (builderClass, builderCtor, activity);
        if (builderObj == nullptr)
        {
            env->DeleteLocalRef (builderClass);
            return {};
        }

        jclass llmClass = env->FindClass ("com/google/android/aicore/llminference/LlmInference");
        if (llmClass == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (builderObj);
            env->DeleteLocalRef (builderClass);
            return {};
        }

        jmethodID buildMethod = env->GetMethodID (builderClass, "build", "()Lcom/google/android/aicore/llminference/LlmInference;");
        if (buildMethod == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (builderClass);
            env->DeleteLocalRef (builderObj);
            env->DeleteLocalRef (llmClass);
            return {};
        }

        jobject inferenceObj = env->CallObjectMethod (builderObj, buildMethod);
        env->DeleteLocalRef (builderObj);
        env->DeleteLocalRef (builderClass);

        if (inferenceObj == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (llmClass);
            return {};
        }

        jmethodID generateMethod = env->GetMethodID (llmClass, "generateResponse", "(Ljava/lang/String;)Ljava/lang/String;");
        if (generateMethod == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (inferenceObj);
            env->DeleteLocalRef (llmClass);
            return {};
        }

        jstring promptJStr = env->NewStringUTF (prompt.toRawUTF8());
        jstring resultJStr = (jstring) env->CallObjectMethod (inferenceObj, generateMethod, promptJStr);
        
        env->DeleteLocalRef (promptJStr);
        env->DeleteLocalRef (inferenceObj);
        env->DeleteLocalRef (llmClass);

        juce::String responseText;
        if (resultJStr != nullptr)
        {
            const char* rawText = env->GetStringUTFChars (resultJStr, nullptr);
            responseText = juce::String::fromUTF8 (rawText);
            env->ReleaseStringUTFChars (resultJStr, rawText);
            env->DeleteLocalRef (resultJStr);
        }

        return responseText;
    }
#endif

    // Robust response parser to handle both JSON and unstructured lists from AI Core
    void parseAiResponse (const juce::String& responseText, HarmonicAdvice& advice)
    {
        auto parsed = juce::JSON::parse (responseText);
        if (parsed.isObject())
        {
            if (auto* obj = parsed.getDynamicObject())
            {
                auto extractArray = [](const juce::var& v, juce::StringArray& dest) {
                    if (auto* arr = v.getArray())
                    {
                        for (auto& item : *arr)
                        {
                            juce::String val = item.toString().trim();
                            if (val.isNotEmpty())
                                dest.add (val);
                        }
                    }
                };

                extractArray (obj->getProperty ("safe"), advice.safe);
                extractArray (obj->getProperty ("tension"), advice.tension);
                extractArray (obj->getProperty ("experimental"), advice.experimental);

                if (advice.safe.size() > 0 || advice.tension.size() > 0 || advice.experimental.size() > 0)
                    return;
            }
        }

        // Unstructured text list fallback parser
        juce::StringArray lines;
        lines.addLines (responseText);
        int currentCategory = 0; // 1 = safe, 2 = tension, 3 = experimental

        for (auto& line : lines)
        {
            juce::String l = line.trim().toLowerCase();
            if (l.contains ("safe") || l.contains ("diatonic") || l.contains ("green"))
                currentCategory = 1;
            else if (l.contains ("tension") || l.contains ("borrowed") || l.contains ("modal") || l.contains ("yellow"))
                currentCategory = 2;
            else if (l.contains ("experimental") || l.contains ("chromatic") || l.contains ("red"))
                currentCategory = 3;
            else if (l.isNotEmpty())
            {
                // Clean common list syntax (e.g. "1. C Maj" or "- C Maj")
                juce::String cleanLine = line.trim();
                while (cleanLine.isNotEmpty() && (cleanLine[0] == '-' || cleanLine[0] == '*' || (cleanLine[0] >= '0' && cleanLine[0] <= '9') || cleanLine[0] == '.' || cleanLine[0] == ' '))
                {
                    cleanLine = cleanLine.substring (1).trim();
                }

                if (cleanLine.isNotEmpty() && cleanLine.length() <= 12) // sanity check length
                {
                    if (currentCategory == 1) advice.safe.add (cleanLine);
                    else if (currentCategory == 2) advice.tension.add (cleanLine);
                    else if (currentCategory == 3) advice.experimental.add (cleanLine);
                }
            }
        }
    }
};
