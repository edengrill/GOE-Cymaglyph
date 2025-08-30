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
    // Handle key releases first
    if (key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
    {
        // Let system shortcuts pass through
        return false;
    }
    
    // Computer keyboard to MIDI mapping
    const char keys[] = "awsedftgyhujkolp;'";
    const int baseNote = 60; // Middle C
    
    // Handle space bar to toggle settings
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        bool isVisible = settingsPanel->isFullyVisible();
        settingsPanel->setVisible(!isVisible, true);
        return true;
    }
    
    // Handle escape to send all notes off
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        // Emergency all notes off
        juce::MidiMessage msg = juce::MidiMessage::allNotesOff(1);
        audioProcessor.handleMidiMessage(msg);
        activeKeyNotes.clear();
        return true;
    }
    
    char keyChar = (char)key.getTextCharacter();
    
    for (int i = 0; i < 18; i++)
    {
        if (keyChar == keys[i])
        {
            int noteNumber = baseNote + i;
            
            // Check if this is a key release (we're checking by seeing if the note is already active)
            if (activeKeyNotes.find(noteNumber) != activeKeyNotes.end())
            {
                // This is a key release (key was already pressed)
                activeKeyNotes.erase(noteNumber);
                
                // Send MIDI note off
                juce::MidiMessage msg = juce::MidiMessage::noteOff(1, noteNumber);
                audioProcessor.handleMidiMessage(msg);
            }
            else
            {
                // This is a key press
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
    
    return false;
}

bool SandWizardAudioProcessorEditor::keyStateChanged(bool isKeyDown)
{
    // Don't use this for note handling - it's unreliable
    // We handle everything in keyPressed instead
    
    if (!isKeyDown && !activeKeyNotes.empty())
    {
        // Safety measure: if we have stuck notes and no keys are pressed,
        // clear everything
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