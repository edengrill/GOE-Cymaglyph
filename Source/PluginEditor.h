#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "EnhancedVisualizer.h"
#include "SettingsPanel.h"
#include <set>

class SandWizardAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer,
                                       private juce::Slider::Listener,
                                       private juce::Button::Listener
{
public:
    SandWizardAudioProcessorEditor(SandWizardAudioProcessor&);
    ~SandWizardAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Keyboard handling
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;

private:
    // Timer callback for updating visualizer
    void timerCallback() override;
    
    // Legacy listeners (kept for compatibility)
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    
    // Legacy helper functions
    void setupParameterControls();
    void setupLayout();
    void updatePresetList();
    void saveImage();
    
    SandWizardAudioProcessor& audioProcessor;
    
    // Main components
    std::unique_ptr<EnhancedVisualizer> visualizer;
    std::unique_ptr<SettingsPanel> settingsPanel;
    
    // Silence detection
    float silenceTimer = 0.0f;
    
    // Keyboard tracking to prevent duplicate notes
    std::set<int> activeKeyNotes;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SandWizardAudioProcessorEditor)
};