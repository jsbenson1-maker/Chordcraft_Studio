#pragma once
#include <JuceHeader.h>

class ThemeManager
{
public:
    // Adding 'inline' forces Visual Studio to compile these directly into any file that includes this header,
    // completely solving the LNK2019 unresolved external symbol error.
    inline static juce::Colour getSystemAccentColor()
    {
        #if JUCE_ANDROID
            // TODO: Inject JNI call to android.R.color.system_accent1_500 here
            return juce::Colour(0xff8b5cf6); // Material Violet placeholder
        #else
            // Desktop Fallback
            return juce::Colour(0xff06b6d4); // Glowing Cyan
        #endif
    }
    
    inline static juce::Colour getSystemAccentDark()
    {
        #if JUCE_ANDROID
            return juce::Colour(0xff6d28d9); // Material Violet Dark
        #else
            return juce::Colour(0xff22d3ee); // Desktop Cyan border
        #endif
    }
};