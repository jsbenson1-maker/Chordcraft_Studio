#pragma once
#include <JuceHeader.h>
#include "ChordArrangement.h"
#include "ThemeManager.h"

class SectionTabBarComponent : public juce::Component, public juce::ChangeListener
{
public:
    SectionTabBarComponent (ChordArrangement& arr) : arrangement (arr)
    {
        arrangement.addChangeListener (this);
        
        addButton.setButtonText ("+");
        addButton.onClick = [this] {
            arrangement.addNewSection();
        };
        addAndMakeVisible (addButton);
        
        updateTabs();
    }
    
    ~SectionTabBarComponent() override
    {
        arrangement.removeChangeListener (this);
    }
    
    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff0f172a)); // Dark slate background matching app theme
        
        // Bottom border line
        g.setColour (juce::Colour (0xff1e293b));
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        addButton.setBounds (bounds.removeFromRight (40).reduced (4));
        
        int numSections = (int) arrangement.sections.size();
        if (numSections == 0) return;
        
        int tabWidth = juce::jlimit (60, 150, bounds.getWidth() / numSections);
        int x = 4;
        
        for (int i = 0; i < numSections && i < tabs.size(); ++i)
        {
            tabs[i]->setBounds (x, 4, tabWidth - 8, getHeight() - 8);
            x += tabWidth;
        }
    }
    
    void changeListenerCallback (juce::ChangeBroadcaster* source) override
    {
        updateTabs();
    }
    
    void updateTabs()
    {
        int numSections = (int) arrangement.sections.size();
        if (tabs.size() != numSections)
        {
            tabs.clear();
            for (int i = 0; i < numSections; ++i)
            {
                auto* t = tabs.add (new TabButton (*this, i));
                addAndMakeVisible (t);
            }
            resized();
        }
        repaint();
        for (auto* t : tabs)
            t->repaint();
    }
    
private:
    class TabButton : public juce::Component, public juce::Timer
    {
    public:
        TabButton (SectionTabBarComponent& owner, int index) 
            : owner (owner), idx (index)
        {}

        ~TabButton() override
        {
            stopTimer();
        }
        
        void paint (juce::Graphics& g) override
        {
            bool isActive = (owner.arrangement.activeSectionIndex == idx);
            auto& sec = owner.arrangement.sections[idx];
            
            // Rounded button background
            g.setColour (isActive ? juce::Colour (0xff4f46e5) : juce::Colour (0xff1e293b)); // Indigo for active, Slate for inactive
            g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
            
            if (! isActive)
            {
                g.setColour (juce::Colour (0xff334155));
                g.drawRoundedRectangle (getLocalBounds().toFloat(), 6.0f, 1.0f);
            }
            
            g.setColour (juce::Colour (0xfff8fafc));
            g.setFont (juce::Font ("Helvetica", 13.0f, isActive ? juce::Font::bold : juce::Font::plain));
            juce::String text = juce::String (sec.sectionName) + " (x" + juce::String (sec.loopCount) + ")";
            g.drawText (text, getLocalBounds(), juce::Justification::centred, true);
        }
        
        void mouseDown (const juce::MouseEvent& event) override
        {
            longPressActive = false;
            if (event.mods.isPopupMenu())
            {
                showContextMenu();
            }
            else if (event.getNumberOfClicks() == 2)
            {
                stopTimer();
                showEditSectionDialog();
            }
            else
            {
                startTimer (500); // Start timer for long press (500ms)
            }
        }
        
        void mouseUp (const juce::MouseEvent& event) override
        {
            if (isTimerRunning())
            {
                stopTimer();
                if (! longPressActive && event.getNumberOfClicks() < 2)
                {
                    owner.arrangement.loadActiveSection (idx);
                }
            }
        }

        void mouseDrag (const juce::MouseEvent& event) override
        {
            if (event.getDistanceFromDragStart() > 10)
                stopTimer();
        }

        void timerCallback() override
        {
            stopTimer();
            longPressActive = true;
            ThemeManager::triggerHapticImpact();
            showContextMenu();
        }

        void mouseDoubleClick (const juce::MouseEvent& event) override
        {
            // Handled in mouseDown to prevent interference from first click selection reloads
        }
        
    private:
        void showContextMenu()
        {
            juce::PopupMenu m;
            m.addItem (1, "Edit Section Settings...");
            m.addItem (2, "Set Loop Count (1)");
            m.addItem (3, "Set Loop Count (2)");
            m.addItem (4, "Set Loop Count (3)");
            m.addItem (5, "Set Loop Count (4)");
            m.addItem (6, "Set Loop Count (8)");
            m.addItem (7, "Delete Section", owner.arrangement.sections.size() > 1);
            
            m.showMenuAsync (juce::PopupMenu::Options(), [this](int result) {
                if (result == 1)
                {
                    showEditSectionDialog();
                }
                else if (result >= 2 && result <= 6)
                {
                    int loops = 1;
                    if (result == 3) loops = 2;
                    else if (result == 4) loops = 3;
                    else if (result == 5) loops = 4;
                    else if (result == 6) loops = 8;
                    
                    owner.arrangement.saveActiveSection();
                    owner.arrangement.sections[idx].loopCount = loops;
                    owner.arrangement.sendProgressionToAudioThread();
                    owner.arrangement.notifyChanges();
                }
                else if (result == 7)
                {
                    owner.arrangement.deleteSection (idx);
                }
            });
        }
        
        void showEditSectionDialog()
        {
            auto* alert = new juce::AlertWindow ("Section Settings", "Modify properties for this section:", juce::AlertWindow::QuestionIcon);
            
            auto& sec = owner.arrangement.sections[idx];
            alert->addTextEditor ("secName", sec.sectionName, "Name:");
            alert->addTextEditor ("loopCount", juce::String (sec.loopCount), "Loop Count:");
            alert->addTextEditor ("secBpm", juce::String (sec.bpm, 1), "BPM:");
            
            alert->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
            if (owner.arrangement.sections.size() > 1)
                alert->addButton ("Delete Section", 2);
            alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            
            alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert](int result) {
                if (result == 1)
                {
                    juce::String newName = alert->getTextEditorContents ("secName").trim();
                    int loops = alert->getTextEditorContents ("loopCount").getIntValue();
                    double newBpm = alert->getTextEditorContents ("secBpm").getDoubleValue();
                    
                    if (loops < 1) loops = 1;
                    if (newBpm < 20.0) newBpm = 20.0;
                    if (newBpm > 300.0) newBpm = 300.0;
                    
                    owner.arrangement.saveActiveSection();
                    
                    if (newName.isNotEmpty())
                        owner.arrangement.sections[idx].sectionName = newName.toStdString();
                        
                    owner.arrangement.sections[idx].loopCount = loops;
                    owner.arrangement.sections[idx].bpm = newBpm;
                    
                    if (owner.arrangement.activeSectionIndex == idx)
                    {
                        owner.arrangement.bpm = (float) newBpm;
                        if (owner.arrangement.getAudioProcessor() != nullptr)
                            owner.arrangement.getAudioProcessor()->setTempo ((float) newBpm);
                    }
                    
                    owner.arrangement.sendProgressionToAudioThread();
                    owner.arrangement.notifyChanges();
                }
                else if (result == 2)
                {
                    owner.arrangement.deleteSection (idx);
                }
                delete alert;
            }), true);
        }
        
        SectionTabBarComponent& owner;
        int idx;
        bool longPressActive = false;
    };
    
    ChordArrangement& arrangement;
    juce::TextButton addButton;
    juce::OwnedArray<TabButton> tabs;
};
