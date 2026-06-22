#pragma once

#include <JuceHeader.h>
#include "ChordArrangement.h"

class TrackMixerComponent : public juce::Component, public juce::ChangeListener
{
public:
    TrackMixerComponent (ChordArrangement& arr);
    ~TrackMixerComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDrag (const juce::MouseEvent& event) override;
    bool keyPressed (const juce::KeyPress& key) override;
    void visibilityChanged() override;

    std::function<void()> onCloseClicked;

private:
    ChordArrangement& arrangement;
    juce::Viewport viewport;

    float startDragX = 0;
    float startDragY = 0;
    bool hasSwipedDismiss = false;

    // Scrolling list content component
    class MixerListContent : public juce::Component
    {
    public:
        MixerListContent (TrackMixerComponent& owner);
        ~MixerListContent() override = default;

        void resized() override;
        void updateTracks();

    private:
        class TrackRow : public juce::Component, public juce::Slider::Listener
        {
        public:
            TrackRow (MixerListContent& owner, int index);
            ~TrackRow() override = default;

            void paint (juce::Graphics& g) override;
            void resized() override;
            void sliderValueChanged (juce::Slider* slider) override;

        private:
            void chooseInstrument();
            void choosePattern();
            juce::String getInstrumentName (int programNum) const;
            juce::String getGMFamilyName (int programNum) const;

            MixerListContent& owner;
            int idx;

            juce::ToggleButton enableToggle;
            juce::Label trackLabel;
            juce::TextButton instrumentButton;
            juce::TextButton patternButton;
            juce::Slider volumeSlider;
            juce::TextButton removeButton;
        };

        TrackMixerComponent& owner;
        juce::OwnedArray<TrackRow> rows;
        juce::TextButton addMelodicButton;
        juce::TextButton addDrumButton;
    };

    MixerListContent listContent;
    juce::TextButton closeButton;

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackMixerComponent)
};
