#include "ControlPanel.h"

ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState& vts)
    : apvts(vts)
{
    // Create filter controls
    filterTypeBox.addItem("Lowpass", 1);
    filterTypeBox.addItem("Highpass", 2);
    filterTypeBox.addItem("Bandpass", 3);
    filterTypeBox.addItem("Notch", 4);
    filterTypeBox.addItem("Off", 5);
    addAndMakeVisible(filterTypeBox);
    filterTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "filterType", filterTypeBox);
    
    // Filter parameters
    filterCutoffSlider = createRotarySlider("filterCutoff");
    filterCutoffLabel = createLabel("Cutoff", filterCutoffSlider.get());
    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "filterCutoff", *filterCutoffSlider);
    
    filterResonanceSlider = createRotarySlider("filterResonance");
    filterResonanceLabel = createLabel("Resonance", filterResonanceSlider.get());
    filterResonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "filterResonance", *filterResonanceSlider);
    
    filterDriveSlider = createRotarySlider("filterDrive");
    filterDriveLabel = createLabel("Drive", filterDriveSlider.get());
    filterDriveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "filterDrive", *filterDriveSlider);
    
    filterEnvSlider = createRotarySlider("filterEnvAmount");
    filterEnvLabel = createLabel("Env Amt", filterEnvSlider.get());
    filterEnvAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "filterEnvAmount", *filterEnvSlider);
    
    // ADSR controls
    attackSlider = createRotarySlider("ampAttack");
    attackLabel = createLabel("Attack", attackSlider.get());
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "ampAttack", *attackSlider);
    
    decaySlider = createRotarySlider("ampDecay");
    decayLabel = createLabel("Decay", decaySlider.get());
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "ampDecay", *decaySlider);
    
    sustainSlider = createRotarySlider("ampSustain");
    sustainLabel = createLabel("Sustain", sustainSlider.get());
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "ampSustain", *sustainSlider);
    
    releaseSlider = createRotarySlider("ampRelease");
    releaseLabel = createLabel("Release", releaseSlider.get());
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "ampRelease", *releaseSlider);
    
    // Effects controls
    reverbMixSlider = createRotarySlider("reverbMix");
    reverbMixLabel = createLabel("Reverb", reverbMixSlider.get());
    reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "reverbMix", *reverbMixSlider);
    
    chorusMixSlider = createRotarySlider("chorusMix");
    chorusMixLabel = createLabel("Chorus", chorusMixSlider.get());
    chorusMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "chorusMix", *chorusMixSlider);
    
    delayMixSlider = createRotarySlider("delayMix");
    delayMixLabel = createLabel("Delay", delayMixSlider.get());
    delayMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "delayMix", *delayMixSlider);
    
    // Master volume
    masterVolumeSlider = createRotarySlider("masterVolume");
    masterVolumeLabel = createLabel("Master", masterVolumeSlider.get());
    masterVolumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "masterVolume", *masterVolumeSlider);
    
    // Start animator
    animator = std::make_unique<AlphaAnimator>(*this);
    animator->startTimerHz(60);
    
    setAlpha(0.0f);
    
    // Make the component not intercept mouse clicks in transparent areas
    setInterceptsMouseClicks(true, true);
}

ControlPanel::~ControlPanel()
{
    animator->stopTimer();
}

void ControlPanel::paint(juce::Graphics& g)
{
    // Don't fill the entire background - only draw section backgrounds
    // This allows clicks to pass through to components below
    
    // Draw section backgrounds
    auto bounds = getLocalBounds().reduced(20);
    
    // Filter section
    auto filterSection = bounds.removeFromTop(150);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f * currentAlpha));
    g.fillRoundedRectangle(filterSection.toFloat(), 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f * currentAlpha));
    g.drawText("FILTER", filterSection.removeFromTop(25), juce::Justification::centred);
    
    bounds.removeFromTop(10);
    
    // ADSR section
    auto adsrSection = bounds.removeFromTop(120);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f * currentAlpha));
    g.fillRoundedRectangle(adsrSection.toFloat(), 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f * currentAlpha));
    g.drawText("ENVELOPE", adsrSection.removeFromTop(25), juce::Justification::centred);
    
    bounds.removeFromTop(10);
    
    // Effects section
    auto effectsSection = bounds.removeFromTop(120);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f * currentAlpha));
    g.fillRoundedRectangle(effectsSection.toFloat(), 10.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f * currentAlpha));
    g.drawText("EFFECTS", effectsSection.removeFromTop(25), juce::Justification::centred);
}

bool ControlPanel::hitTest(int x, int y)
{
    // Only respond to clicks if the panel is visible
    if (currentAlpha < 0.1f)
        return false;
    
    // Only respond to clicks in the control panel area (top 450 pixels)
    if (y > 450)
        return false;
    
    return true;
}

void ControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Filter section
    auto filterSection = bounds.removeFromTop(150);
    filterSection.removeFromTop(30); // Title space
    
    auto filterRow1 = filterSection.removeFromTop(40);
    filterTypeBox.setBounds(filterRow1.removeFromLeft(120).reduced(5));
    
    auto filterRow2 = filterSection.removeFromTop(80);
    int knobSize = 60;
    int labelHeight = 20;
    
    auto cutoffBounds = filterRow2.removeFromLeft(knobSize + 20);
    filterCutoffSlider->setBounds(cutoffBounds.removeFromTop(knobSize).reduced(5));
    filterCutoffLabel->setBounds(cutoffBounds);
    
    auto resBounds = filterRow2.removeFromLeft(knobSize + 20);
    filterResonanceSlider->setBounds(resBounds.removeFromTop(knobSize).reduced(5));
    filterResonanceLabel->setBounds(resBounds);
    
    auto driveBounds = filterRow2.removeFromLeft(knobSize + 20);
    filterDriveSlider->setBounds(driveBounds.removeFromTop(knobSize).reduced(5));
    filterDriveLabel->setBounds(driveBounds);
    
    auto envBounds = filterRow2.removeFromLeft(knobSize + 20);
    filterEnvSlider->setBounds(envBounds.removeFromTop(knobSize).reduced(5));
    filterEnvLabel->setBounds(envBounds);
    
    bounds.removeFromTop(10);
    
    // ADSR section
    auto adsrSection = bounds.removeFromTop(120);
    adsrSection.removeFromTop(30); // Title space
    
    auto adsrRow = adsrSection.removeFromTop(80);
    
    auto attackBounds = adsrRow.removeFromLeft(knobSize + 20);
    attackSlider->setBounds(attackBounds.removeFromTop(knobSize).reduced(5));
    attackLabel->setBounds(attackBounds);
    
    auto decayBounds = adsrRow.removeFromLeft(knobSize + 20);
    decaySlider->setBounds(decayBounds.removeFromTop(knobSize).reduced(5));
    decayLabel->setBounds(decayBounds);
    
    auto sustainBounds = adsrRow.removeFromLeft(knobSize + 20);
    sustainSlider->setBounds(sustainBounds.removeFromTop(knobSize).reduced(5));
    sustainLabel->setBounds(sustainBounds);
    
    auto releaseBounds = adsrRow.removeFromLeft(knobSize + 20);
    releaseSlider->setBounds(releaseBounds.removeFromTop(knobSize).reduced(5));
    releaseLabel->setBounds(releaseBounds);
    
    bounds.removeFromTop(10);
    
    // Effects section
    auto effectsSection = bounds.removeFromTop(120);
    effectsSection.removeFromTop(30); // Title space
    
    auto effectsRow = effectsSection.removeFromTop(80);
    
    auto reverbBounds = effectsRow.removeFromLeft(knobSize + 20);
    reverbMixSlider->setBounds(reverbBounds.removeFromTop(knobSize).reduced(5));
    reverbMixLabel->setBounds(reverbBounds);
    
    auto chorusBounds = effectsRow.removeFromLeft(knobSize + 20);
    chorusMixSlider->setBounds(chorusBounds.removeFromTop(knobSize).reduced(5));
    chorusMixLabel->setBounds(chorusBounds);
    
    auto delayBounds = effectsRow.removeFromLeft(knobSize + 20);
    delayMixSlider->setBounds(delayBounds.removeFromTop(knobSize).reduced(5));
    delayMixLabel->setBounds(delayBounds);
    
    auto masterBounds = effectsRow.removeFromLeft(knobSize + 40);
    masterBounds.removeFromLeft(20); // Spacing
    masterVolumeSlider->setBounds(masterBounds.removeFromTop(knobSize).reduced(5));
    masterVolumeLabel->setBounds(masterBounds);
}

void ControlPanel::setVisible(bool shouldBeVisible, bool animate)
{
    targetAlpha = shouldBeVisible ? 1.0f : 0.0f;
    
    if (!animate)
    {
        currentAlpha = targetAlpha;
        setAlpha(currentAlpha);
    }
}

std::unique_ptr<juce::Slider> ControlPanel::createRotarySlider(const juce::String& paramID)
{
    auto slider = std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag, 
                                                  juce::Slider::TextBoxBelow);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
    slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::grey);
    slider->setColour(juce::Slider::thumbColourId, juce::Colours::white);
    slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    
    addAndMakeVisible(slider.get());
    return slider;
}

std::unique_ptr<juce::Label> ControlPanel::createLabel(const juce::String& text, juce::Slider* slider)
{
    auto label = std::make_unique<juce::Label>(text, text);
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    label->attachToComponent(slider, false);
    addAndMakeVisible(label.get());
    return label;
}

void ControlPanel::AlphaAnimator::timerCallback()
{
    if (std::abs(panel.currentAlpha - panel.targetAlpha) > 0.01f)
    {
        panel.currentAlpha += (panel.targetAlpha - panel.currentAlpha) * 0.15f;
        panel.setAlpha(panel.currentAlpha);
        panel.repaint();
    }
}