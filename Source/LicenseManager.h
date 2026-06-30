#pragma once
#include <JuceHeader.h>

class LicenseManager : public juce::ChangeBroadcaster
{
public:
    static LicenseManager* getInstance()
    {
        static LicenseManager instance;
        return &instance;
    }

    bool isPro() const { return proUnlocked; }
    
    void setPro (bool isNowPro) 
    { 
        proUnlocked = isNowPro; 
        sendChangeMessage(); // Alert UI to remove paywalls instantly
    }

    // --- JNI Triggers (C++ commanding Android) ---
    void showStartupAd()
    {
        #if JUCE_ANDROID
        callJavaMethod("showStartupAd");
        #endif
    }

    void initiateProPurchase()
    {
        #if JUCE_ANDROID
        callJavaMethod("initiateProPurchase");
        #else
        // Desktop testing fallback
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon, "Desktop Simulation", "Google Play Billing triggered.\nPro Tier Unlocked!", "OK");
        setPro (true);
        #endif
    }

private:
    LicenseManager() = default;
    bool proUnlocked = false; // Default to Free Tier on launch

    #if JUCE_ANDROID
    void callJavaMethod (const char* methodName)
    {
        if (auto* env = juce::getEnv())
        {
            auto activity = juce::getAppContext();
            if (activity.get() != nullptr)
            {
                jclass clazz = env->GetObjectClass(activity.get());
                jmethodID methodId = env->GetMethodID(clazz, methodName, "()V");
                if (methodId != nullptr) {
                    env->CallVoidMethod(activity.get(), methodId);
                }
                env->DeleteLocalRef(clazz);
            }
        }
    }
    #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LicenseManager)
};

// --- JNI Callback (Android commanding C++) ---
#if JUCE_ANDROID
extern "C" inline JNIEXPORT void JNICALL
Java_com_groovespeakstudios_chordcraftstudio_MainActivity_nativeSetProStatus(JNIEnv* env, jobject thiz, jboolean isPro)
{
    juce::MessageManager::callAsync([isPro]() {
        LicenseManager::getInstance()->setPro((bool)isPro);
    });
}
#endif
