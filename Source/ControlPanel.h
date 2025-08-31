#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class ControlPanel : public juce::Component
{
public:
    ControlPanel(juce::AudioProcessorValueTreeState& apvts);
    ~ControlPanel() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    bool hitTest(int x, int y) override;
    
    void setVisible(bool shouldBeVisible, bool animate = true);
    bool isFullyVisible() const { return targetAlpha > 0.5f; }

private:
    // Create a rotary slider with label
    std::unique_ptr<juce::Slider> createRotarySlider(const juce::String& paramID);
    std::unique_ptr<juce::Label> createLabel(const juce::String& text, juce::Slider* slider);
    
    // Parameter attachments
    juce::AudioProcessorValueTreeState& apvts;
    
    // Filter controls
    juce::ComboBox filterTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterTypeAttachment;
    
    std::unique_ptr<juce::Slider> filterCutoffSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::Label> filterCutoffLabel;
    
    std::unique_ptr<juce::Slider> filterResonanceSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<juce::Label> filterResonanceLabel;
    
    std::unique_ptr<juce::Slider> filterDriveSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterDriveAttachment;
    std::unique_ptr<juce::Label> filterDriveLabel;
    
    std::unique_ptr<juce::Slider> filterEnvSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvAttachment;
    std::unique_ptr<juce::Label> filterEnvLabel;
    
    // ADSR controls
    std::unique_ptr<juce::Slider> attackSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::Label> attackLabel;
    
    std::unique_ptr<juce::Slider> decaySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::Label> decayLabel;
    
    std::unique_ptr<juce::Slider> sustainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::Label> sustainLabel;
    
    std::unique_ptr<juce::Slider> releaseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::Label> releaseLabel;
    
    // Effects controls
    std::unique_ptr<juce::Slider> reverbMixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<juce::Label> reverbMixLabel;
    
    std::unique_ptr<juce::Slider> chorusMixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chorusMixAttachment;
    std::unique_ptr<juce::Label> chorusMixLabel;
    
    std::unique_ptr<juce::Slider> delayMixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayMixAttachment;
    std::unique_ptr<juce::Label> delayMixLabel;
    
    // Master controls
    std::unique_ptr<juce::Slider> masterVolumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolumeAttachment;
    std::unique_ptr<juce::Label> masterVolumeLabel;
    
    // Animation
    float currentAlpha = 0.0f;
    float targetAlpha = 0.0f;
    
    class AlphaAnimator : public juce::Timer
    {
    public:
        AlphaAnimator(ControlPanel& p) : panel(p) {}
        void timerCallback() override;
    private:
        ControlPanel& panel;
    };
    
    std::unique_ptr<AlphaAnimator> animator;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};