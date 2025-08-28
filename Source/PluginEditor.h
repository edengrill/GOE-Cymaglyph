#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "EnhancedVisualizer.h"
#include "SettingsPanel.h"

class GOECymaglyphAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer,
                                         private juce::Slider::Listener,
                                         private juce::Button::Listener
{
public:
    GOECymaglyphAudioProcessorEditor(GOECymaglyphAudioProcessor&);
    ~GOECymaglyphAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Keyboard handling (not used in v3.0)
    bool keyPressed(const juce::KeyPress& key) override;

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
    
    GOECymaglyphAudioProcessor& audioProcessor;
    
    // Main components
    std::unique_ptr<EnhancedVisualizer> visualizer;
    std::unique_ptr<SettingsPanel> settingsPanel;
    
    // Silence detection
    float silenceTimer = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GOECymaglyphAudioProcessorEditor)
};