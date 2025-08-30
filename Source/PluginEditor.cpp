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
    settingsPanel->setVisible(false, false);  // Start hidden
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
    
    // Request keyboard focus for computer keyboard MIDI
    setWantsKeyboardFocus(true);
    
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
    // Computer keyboard to MIDI mapping
    const char keys[] = "awsedftgyhujkolp;'";
    const int baseNote = 60; // Middle C
    
    char keyChar = (char)key.getTextCharacter();
    
    for (int i = 0; i < 18; i++)
    {
        if (keyChar == keys[i])
        {
            int noteNumber = baseNote + i;
            
            // Only send note-on if this note isn't already active
            if (activeKeyNotes.find(noteNumber) == activeKeyNotes.end())
            {
                activeKeyNotes.insert(noteNumber);
                
                // Send MIDI note on
                juce::MidiMessage msg = juce::MidiMessage::noteOn(1, noteNumber, (juce::uint8)100);
                audioProcessor.handleMidiMessage(msg);
                
                // Hide settings panel when playing
                if (settingsPanel->isFullyVisible())
                {
                    settingsPanel->setVisible(false, true);
                }
                silenceTimer = 0.0f;
            }
            
            return true;
        }
    }
    
    // Handle space bar to toggle settings
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        bool isVisible = settingsPanel->isFullyVisible();
        settingsPanel->setVisible(!isVisible, true);
        return true;
    }
    
    return false;
}

bool SandWizardAudioProcessorEditor::keyStateChanged(bool isKeyDown)
{
    if (!isKeyDown)  // Key released
    {
        // Check which key was released and send note-off for that specific note
        const char keys[] = "awsedftgyhujkolp;'";
        const int baseNote = 60; // Middle C
        
        // Get current key states
        auto currentPressedKeys = juce::Desktop::getInstance().getMouseSource(0)->getCurrentModifiers();
        
        // Check each possible key
        for (int i = 0; i < 18; i++)
        {
            int noteNumber = baseNote + i;
            
            // If this note was active but the key is no longer pressed
            if (activeKeyNotes.find(noteNumber) != activeKeyNotes.end())
            {
                // Check if the key for this note is still pressed
                bool keyStillPressed = false;
                
                // Since we can't directly check individual keys, we'll send note-off
                // for all previously active notes and clear the set
                // This is a simpler approach that ensures no stuck notes
            }
        }
        
        // Send note-off for all active notes and clear the set
        for (int noteNumber : activeKeyNotes)
        {
            juce::MidiMessage msg = juce::MidiMessage::noteOff(1, noteNumber);
            audioProcessor.handleMidiMessage(msg);
        }
        activeKeyNotes.clear();
    }
    
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