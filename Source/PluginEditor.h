#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Visualizer.h"

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

private:
    // Timer callback for updating visualizer frequency
    void timerCallback() override;
    
    // Listeners
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    
    // Helper functions
    void setupParameterControls();
    void setupLayout();
    void updatePresetList();
    void saveImage();
    
    GOECymaglyphAudioProcessor& audioProcessor;
    
    // Main visualizer
    std::unique_ptr<CymaglyphVisualizer> visualizer;
    
    // Audio controls
    juce::Slider freqSlider;
    juce::Label freqLabel;
    juce::TextEditor freqTextEntry;
    
    juce::Slider gainSlider;
    juce::Label gainLabel;
    
    juce::ComboBox waveformSelector;
    juce::Label waveformLabel;
    
    juce::ToggleButton gateButton;
    
    juce::Slider a4RefSlider;
    juce::Label a4RefLabel;
    
    // Sweep controls
    juce::GroupComponent sweepGroup;
    juce::ToggleButton sweepEnableButton;
    juce::Slider sweepRateSlider;
    juce::Label sweepRateLabel;
    juce::Slider sweepRangeSlider;
    juce::Label sweepRangeLabel;
    
    // Visual controls
    juce::GroupComponent visualGroup;
    
    juce::ComboBox mediumSelector;
    juce::Label mediumLabel;
    
    juce::ComboBox geometrySelector;
    juce::Label geometryLabel;
    
    juce::ComboBox mountingSelector;
    juce::Label mountingLabel;
    
    juce::Slider accuracySlider;
    juce::Label accuracyLabel;
    
    juce::Slider nodeThresholdSlider;
    juce::Label nodeThresholdLabel;
    
    juce::Slider grainAmountSlider;
    juce::Label grainAmountLabel;
    
    juce::ComboBox colorModeSelector;
    juce::Label colorModeLabel;
    
    juce::TextButton resetAccumButton;
    juce::TextButton saveImageButton;
    
    // Preset controls
    juce::ComboBox presetSelector;
    juce::TextButton savePresetButton;
    juce::Label presetLabel;
    
    // About footer
    juce::Label aboutLabel;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a4RefAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sweepEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sweepRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sweepRangeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mediumAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> geomAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> accuracyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> nodeEpsAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainAmtAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> colorModeAttachment;
    
    // Look and feel
    juce::LookAndFeel_V4 lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GOECymaglyphAudioProcessorEditor)
};