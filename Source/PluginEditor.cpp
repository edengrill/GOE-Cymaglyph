#include "PluginProcessor.h"
#include "PluginEditor.h"

GOECymaglyphAudioProcessorEditor::GOECymaglyphAudioProcessorEditor(GOECymaglyphAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Create visualizer that fills the entire window
    visualizer = std::make_unique<CymaglyphVisualizer>(audioProcessor.getAPVTS());
    addAndMakeVisible(visualizer.get());
    
    // Set editor size
    setSize(800, 800);
    setResizable(true, true);
    setResizeLimits(400, 400, 1920, 1920);
    
    // Make this component want keyboard focus
    setWantsKeyboardFocus(true);
    
    // Start timer to update visualizer
    startTimerHz(30);
    
    // Show help on first launch
    if (!hasShownHelp)
    {
        showHelpOverlay = true;
        hasShownHelp = true;
    }
}

GOECymaglyphAudioProcessorEditor::~GOECymaglyphAudioProcessorEditor()
{
    stopTimer();
}

void GOECymaglyphAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    
    // Draw mode indicator in top left
    g.setFont(16.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    
    juce::String modeText;
    switch (audioProcessor.getCurrentMode())
    {
        case GOECymaglyphAudioProcessor::Neutral:
            modeText = "NEUTRAL - Use arrows & shift";
            break;
        case GOECymaglyphAudioProcessor::Monophonic:
            modeText = "MONOPHONIC - Play single notes";
            break;
        case GOECymaglyphAudioProcessor::Polyphonic:
            modeText = "POLYPHONIC - Play chords";
            break;
    }
    
    g.drawText(modeText, 10, 10, 300, 25, juce::Justification::left);
    
    // Draw help overlay if active
    if (showHelpOverlay)
    {
        // Semi-transparent background
        g.setColour(juce::Colours::black.withAlpha(0.85f));
        g.fillAll();
        
        // Help text
        auto bounds = getLocalBounds();
        auto helpArea = bounds.reduced(50);
        
        g.setColour(juce::Colours::white);
        g.setFont(24.0f);
        g.drawText("GOE CYMAGLYPH - Controls", helpArea.removeFromTop(40), juce::Justification::centred);
        
        g.setFont(18.0f);
        helpArea.removeFromTop(30);
        
        juce::String helpText = 
            "RETURN/ENTER - Cycle through modes\n\n"
            "NEUTRAL MODE:\n"
            "  SHIFT - Toggle sound on/off\n"
            "  ← → ARROWS - Control frequency (20Hz - 20kHz)\n\n"
            "MONOPHONIC MODE:\n"
            "  Play single notes on your MIDI keyboard\n\n"
            "POLYPHONIC MODE:\n"
            "  Play multiple notes simultaneously\n\n"
            "ESC - Toggle this help screen\n\n"
            "Press any key to continue...";
            
        g.drawMultiLineText(helpText, helpArea.getX(), helpArea.getY() + 30, helpArea.getWidth());
    }
}

void GOECymaglyphAudioProcessorEditor::resized()
{
    // Visualizer fills entire window
    visualizer->setBounds(getLocalBounds());
}

void GOECymaglyphAudioProcessorEditor::timerCallback()
{
    // Update visualizer with current frequency/frequencies
    if (audioProcessor.getCurrentMode() == GOECymaglyphAudioProcessor::Polyphonic)
    {
        auto frequencies = audioProcessor.getActiveFrequencies();
        if (!frequencies.empty())
        {
            // For now, visualize the average or dominant frequency
            float avgFreq = 0.0f;
            for (float f : frequencies)
                avgFreq += f;
            avgFreq /= frequencies.size();
            visualizer->setFrequency(avgFreq);
        }
    }
    else
    {
        visualizer->setFrequency(audioProcessor.getCurrentFrequency());
    }
    
    repaint();
}

bool GOECymaglyphAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    // Hide help overlay on any key if showing
    if (showHelpOverlay)
    {
        showHelpOverlay = false;
        repaint();
        return true;
    }
    
    // Return/Enter - cycle modes
    if (key.isKeyCode(juce::KeyPress::returnKey))
    {
        audioProcessor.cycleMode();
        repaint();
        return true;
    }
    
    // ESC - toggle help
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        showHelpOverlay = !showHelpOverlay;
        repaint();
        return true;
    }
    
    // Neutral mode controls
    if (audioProcessor.getCurrentMode() == GOECymaglyphAudioProcessor::Neutral)
    {
        // Shift - toggle gate (just check for shift key, not as modifier)
        if (key.getKeyCode() == juce::KeyPress::createFromDescription("shift").getKeyCode())
        {
            audioProcessor.toggleGate();
            return true;
        }
        
        // Left arrow - decrease frequency
        if (key.isKeyCode(juce::KeyPress::leftKey))
        {
            audioProcessor.adjustFrequency(-1.0f);
            return true;
        }
        
        // Right arrow - increase frequency  
        if (key.isKeyCode(juce::KeyPress::rightKey))
        {
            audioProcessor.adjustFrequency(1.0f);
            return true;
        }
    }
    
    return false;
}

void GOECymaglyphAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    // No longer needed
}

void GOECymaglyphAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    // No longer needed
}

void GOECymaglyphAudioProcessorEditor::setupParameterControls()
{
    // No longer needed - all controls removed
}

void GOECymaglyphAudioProcessorEditor::setupLayout()
{
    // No longer needed
}

void GOECymaglyphAudioProcessorEditor::updatePresetList()
{
    // No longer needed
}

void GOECymaglyphAudioProcessorEditor::saveImage()
{
    // Could keep this for screenshot functionality later
}