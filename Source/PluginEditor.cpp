#include "PluginProcessor.h"
#include "PluginEditor.h"

SandWizardAudioProcessorEditor::SandWizardAudioProcessorEditor(SandWizardAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Create enhanced visualizer
    visualizer = std::make_unique<EnhancedVisualizer>(audioProcessor.getAPVTS());
    addAndMakeVisible(visualizer.get());
    
    // Create settings panel
    settingsPanel = std::make_unique<SettingsPanel>();
    settingsPanel->setVisible(true, false);  // Start visible immediately
    addAndMakeVisible(settingsPanel.get());
    
    // Setup callbacks
    settingsPanel->onModeSelected = [this](int mode) {
        audioProcessor.setSynthMode(mode);
        visualizer->setSynthMode(mode);
    };
    
    settingsPanel->onMonoPolyChanged = [this](bool mono) {
        audioProcessor.setMonophonic(mono);
    };
    
    settingsPanel->onOctaveChanged = [this](int octaveShift) {
        audioProcessor.setOctaveShift(octaveShift);
    };
    
    // Set editor size
    setSize(900, 900);
    setResizable(true, true);
    setResizeLimits(600, 600, 1920, 1920);
    
    // Start timer to check for silence and update visualizer
    startTimerHz(60);
}

SandWizardAudioProcessorEditor::~SandWizardAudioProcessorEditor()
{
    stopTimer();
}

void SandWizardAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void SandWizardAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Both components fill the entire window
    visualizer->setBounds(bounds);
    settingsPanel->setBounds(bounds);
}

void SandWizardAudioProcessorEditor::timerCallback()
{
    // Check if audio is playing
    bool isPlaying = audioProcessor.isPlaying();
    
    // Update visualizer state
    if (audioProcessor.getMonophonic())
    {
        visualizer->setFrequency(audioProcessor.getCurrentFrequency());
    }
    else
    {
        auto frequencies = audioProcessor.getActiveFrequencies();
        visualizer->setFrequencies(frequencies);
    }
    
    visualizer->setActive(isPlaying);
    
    // Update silence timer
    if (!isPlaying)
    {
        silenceTimer += 1.0f / 60.0f;
        
        // Show settings after 0.5 seconds of silence
        if (silenceTimer > 0.5f && !settingsPanel->isFullyVisible())
        {
            settingsPanel->setVisible(true, true);
        }
    }
    else
    {
        silenceTimer = 0.0f;
        
        // Hide settings immediately when playing
        if (settingsPanel->isFullyVisible())
        {
            settingsPanel->setVisible(false, false);
        }
    }
}

// Legacy functions (kept for compatibility but not used)
bool SandWizardAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    return false;
}

void SandWizardAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    juce::ignoreUnused(slider);
}

void SandWizardAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    juce::ignoreUnused(button);
}

void SandWizardAudioProcessorEditor::setupParameterControls()
{
}

void SandWizardAudioProcessorEditor::setupLayout()
{
}

void SandWizardAudioProcessorEditor::updatePresetList()
{
}

void SandWizardAudioProcessorEditor::saveImage()
{
}