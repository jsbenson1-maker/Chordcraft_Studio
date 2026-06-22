#pragma once
#if JUCE_ANDROID
 #define JUCE_CORE_INCLUDE_JNI_HELPERS 1
#endif
#include <JuceHeader.h>

#if JUCE_ANDROID
#include <jni.h>
#endif

class ThemeManager
{
public:
    // Adding 'inline' forces Visual Studio to compile these directly into any file that includes this header,
    // completely solving the LNK2019 unresolved external symbol error.
    inline static juce::Colour getSystemAccentColor()
    {
        #if JUCE_ANDROID
            return getAndroidMaterialColor ("system_accent1_500", juce::Colour (0xff8b5cf6));
        #else
            // Desktop Fallback
            return juce::Colour (0xff06b6d4); // Glowing Cyan
        #endif
    }
    
    inline static juce::Colour getSystemAccentDark()
    {
        #if JUCE_ANDROID
            return getAndroidMaterialColor ("system_accent1_800", juce::Colour (0xff6d28d9));
        #else
            // Desktop Fallback
            return juce::Colour (0xff22d3ee); // Desktop Cyan border
        #endif
    }

    inline static void triggerHapticRatchet()
    {
        #if JUCE_ANDROID
            triggerAndroidHaptic (16); // HapticFeedbackConstants.CLOCK_TICK = 16
        #endif
    }

    inline static void triggerHapticImpact()
    {
        #if JUCE_ANDROID
            triggerAndroidHaptic (3); // HapticFeedbackConstants.KEYBOARD_TAP = 3
        #endif
    }

private:
    #if JUCE_ANDROID
    // JNI reflection to extract wallpaper colors from android.R.color
    inline static juce::Colour getAndroidMaterialColor (const char* colorResourceName, juce::Colour fallbackColor)
    {
        JNIEnv* env = juce::getEnv();
        if (env == nullptr)
            return fallbackColor;

        auto activityLocalRef = juce::getCurrentActivity();
        jobject activity = activityLocalRef.get();
        if (activity == nullptr)
            return fallbackColor;

        jclass rColorClass = env->FindClass ("android/R$color");
        if (rColorClass == nullptr)
        {
            env->ExceptionClear();
            return fallbackColor;
        }

        jfieldID fieldId = env->GetStaticFieldID (rColorClass, colorResourceName, "I");
        if (fieldId == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (rColorClass);
            return fallbackColor;
        }

        jint resourceId = env->GetStaticIntField (rColorClass, fieldId);
        env->DeleteLocalRef (rColorClass);

        jclass activityClass = env->GetObjectClass (activity);
        if (activityClass == nullptr)
            return fallbackColor;

        jmethodID getColorMethod = env->GetMethodID (activityClass, "getColor", "(I)I");
        if (getColorMethod == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (activityClass);
            return fallbackColor;
        }

        jint colorInt = env->CallIntMethod (activity, getColorMethod, resourceId);
        env->DeleteLocalRef (activityClass);

        return juce::Colour (static_cast<juce::uint32> (colorInt));
    }

    // JNI reflection to invoke performHapticFeedback on the DecorView of the Window
    inline static void triggerAndroidHaptic (int feedbackConstant)
    {
        JNIEnv* env = juce::getEnv();
        if (env == nullptr)
            return;

        auto activityLocalRef = juce::getCurrentActivity();
        jobject activity = activityLocalRef.get();
        if (activity == nullptr)
            return;

        jclass activityClass = env->GetObjectClass (activity);
        if (activityClass == nullptr)
            return;

        jmethodID getWindowMethod = env->GetMethodID (activityClass, "getWindow", "()Landroid/view/Window;");
        if (getWindowMethod == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (activityClass);
            return;
        }

        jobject window = env->CallObjectMethod (activity, getWindowMethod);
        env->DeleteLocalRef (activityClass);
        if (window == nullptr)
            return;

        jclass windowClass = env->GetObjectClass (window);
        if (windowClass == nullptr)
        {
            env->DeleteLocalRef (window);
            return;
        }

        jmethodID getDecorViewMethod = env->GetMethodID (windowClass, "getDecorView", "()Landroid/view/View;");
        if (getDecorViewMethod == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (window);
            env->DeleteLocalRef (windowClass);
            return;
        }

        jobject decorView = env->CallObjectMethod (window, getDecorViewMethod);
        env->DeleteLocalRef (window);
        env->DeleteLocalRef (windowClass);
        if (decorView == nullptr)
            return;

        jclass viewClass = env->GetObjectClass (decorView);
        if (viewClass == nullptr)
        {
            env->DeleteLocalRef (decorView);
            return;
        }

        jmethodID performHapticFeedbackMethod = env->GetMethodID (viewClass, "performHapticFeedback", "(I)Z");
        if (performHapticFeedbackMethod == nullptr)
        {
            env->ExceptionClear();
            env->DeleteLocalRef (decorView);
            env->DeleteLocalRef (viewClass);
            return;
        }

        env->CallBooleanMethod (decorView, performHapticFeedbackMethod, feedbackConstant);
        
        env->DeleteLocalRef (decorView);
        env->DeleteLocalRef (viewClass);
    }
    #endif
};