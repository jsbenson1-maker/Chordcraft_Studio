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
        auto* env = juce::getEnv();
        if (env == nullptr) return;
        
        jobject activity = juce::getAppContext().get();
        if (activity == nullptr) return;
        
        jclass activityClass = env->GetObjectClass(activity);
        jmethodID methodId = env->GetMethodID(activityClass, methodName, "()V");
        
        if (methodId != 0) {
            env->CallVoidMethod(activity, methodId);
        }
        
        env->DeleteLocalRef(activityClass);
    }
    #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LicenseManager)
};

// --- JNI Callback (Android commanding C++) ---
#if JUCE_ANDROID
extern "C" JNIEXPORT void JNICALL
Java_com_groovespeakstudios_chordcraftstudio_MainActivity_nativeSetProStatus(JNIEnv* env, jobject thiz, jboolean isPro)
{
    juce::MessageManager::callAsync([isPro]() {
        LicenseManager::getInstance()->setPro((bool)isPro);
    });
}
#endif
