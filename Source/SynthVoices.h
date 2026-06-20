#pragma once

#include <JuceHeader.h>
#include <cmath>

class SineWaveSound : public juce::SynthesiserSound
{
public:
    SineWaveSound() {}
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    SineWaveVoice() {}
    
    bool canPlaySound (juce::SynthesiserSound* sound) override 
    { 
        return dynamic_cast<SineWaveSound*> (sound) != nullptr; 
    }
    
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        currentAngle = 0.0;
        angleDelta = juce::double_Pi * 2.0 * juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber) / getSampleRate();
        level = velocity * 0.12f; // Keep levels safe to prevent clipping
        tailOff = 0.0;
    }
    
    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }
    
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0)
            {
                for (int s = 0; s < numSamples; ++s)
                {
                    auto val = (float) (std::sin (currentAngle) * level * tailOff);
                    for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
                        outputBuffer.addSample (c, startSample + s, val);
                        
                    currentAngle += angleDelta;
                    tailOff *= 0.99; // Simple exponential decay
                    
                    if (tailOff < 0.005)
                    {
                        clearCurrentNote();
                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                for (int s = 0; s < numSamples; ++s)
                {
                    auto val = (float) (std::sin (currentAngle) * level);
                    for (int c = 0; c < outputBuffer.getNumChannels(); ++c)
                        outputBuffer.addSample (c, startSample + s, val);
                        
                    currentAngle += angleDelta;
                }
            }
        }
    }
    
private:
    double currentAngle = 0.0;
    double angleDelta = 0.0;
    double level = 0.0;
    double tailOff = 0.0;
};
