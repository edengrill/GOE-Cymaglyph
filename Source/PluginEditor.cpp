#include "PluginProcessor.h"
#include "PluginEditor.h"

GOECymaglyphAudioProcessorEditor::GOECymaglyphAudioProcessorEditor(GOECymaglyphAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set look and feel
    lookAndFeel.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    lookAndFeel.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha(0.5f));
    lookAndFeel.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::cyan.withAlpha(0.3f));
    setLookAndFeel(&lookAndFeel);
    
    // Create visualizer
    visualizer = std::make_unique<CymaglyphVisualizer>(audioProcessor.getAPVTS());
    addAndMakeVisible(visualizer.get());
    
    // Setup all controls
    setupParameterControls();
    setupLayout();
    
    // Set editor size
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(800, 600, 1920, 1200);
    
    // Start timer to update visualizer frequency
    startTimerHz(30);
}

GOECymaglyphAudioProcessorEditor::~GOECymaglyphAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void GOECymaglyphAudioProcessorEditor::setupParameterControls()
{
    auto& apvts = audioProcessor.getAPVTS();
    
    // Frequency control
    freqSlider.setSliderStyle(juce::Slider::Rotary);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    freqSlider.setRange(20.0, 20000.0);
    freqSlider.setSkewFactorFromMidPoint(1000.0);
    freqSlider.setTextValueSuffix(" Hz");
    freqSlider.setNumDecimalPlacesToDisplay(1);
    addAndMakeVisible(freqSlider);
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "freq", freqSlider);
    
    freqLabel.setText("Frequency", juce::dontSendNotification);
    freqLabel.attachToComponent(&freqSlider, false);
    freqLabel.setJustificationType(juce::Justification::centredTop);
    addAndMakeVisible(freqLabel);
    
    // Frequency text entry
    freqTextEntry.setMultiLine(false);
    freqTextEntry.setReturnKeyStartsNewLine(false);
    freqTextEntry.setText("440.0");
    freqTextEntry.onReturnKey = [this] {
        auto freq = freqTextEntry.getText().getFloatValue();
        freq = juce::jlimit(20.0f, 20000.0f, freq);
        freqSlider.setValue(freq);
    };
    addAndMakeVisible(freqTextEntry);
    
    // Gain control
    gainSlider.setSliderStyle(juce::Slider::Rotary);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    gainSlider.setRange(0.0, 1.0);
    gainSlider.setTextValueSuffix("");
    gainSlider.setNumDecimalPlacesToDisplay(2);
    addAndMakeVisible(gainSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "gain", gainSlider);
    
    gainLabel.setText("Gain", juce::dontSendNotification);
    gainLabel.attachToComponent(&gainSlider, false);
    gainLabel.setJustificationType(juce::Justification::centredTop);
    addAndMakeVisible(gainLabel);
    
    // Waveform selector
    waveformSelector.addItem("Sine", 1);
    waveformSelector.addItem("Triangle", 2);
    waveformSelector.addItem("Square", 3);
    addAndMakeVisible(waveformSelector);
    waveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "wave", waveformSelector);
    
    waveformLabel.setText("Waveform", juce::dontSendNotification);
    waveformLabel.attachToComponent(&waveformSelector, false);
    addAndMakeVisible(waveformLabel);
    
    // Gate button
    gateButton.setButtonText("Gate");
    addAndMakeVisible(gateButton);
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "gate", gateButton);
    
    // A4 Reference
    a4RefSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    a4RefSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    a4RefSlider.setRange(415.0, 466.0);
    a4RefSlider.setTextValueSuffix(" Hz");
    addAndMakeVisible(a4RefSlider);
    a4RefAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "a4ref", a4RefSlider);
    
    a4RefLabel.setText("A4 Ref", juce::dontSendNotification);
    a4RefLabel.attachToComponent(&a4RefSlider, true);
    addAndMakeVisible(a4RefLabel);
    
    // Sweep controls
    sweepGroup.setText("Sweep");
    addAndMakeVisible(sweepGroup);
    
    sweepEnableButton.setButtonText("Enable");
    addAndMakeVisible(sweepEnableButton);
    sweepEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, "sweepOn", sweepEnableButton);
    
    sweepRateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sweepRateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    sweepRateSlider.setRange(0.01, 10.0);
    sweepRateSlider.setSkewFactorFromMidPoint(1.0);
    sweepRateSlider.setTextValueSuffix(" Hz");
    addAndMakeVisible(sweepRateSlider);
    sweepRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sweepRate", sweepRateSlider);
    
    sweepRateLabel.setText("Rate", juce::dontSendNotification);
    sweepRateLabel.attachToComponent(&sweepRateSlider, true);
    addAndMakeVisible(sweepRateLabel);
    
    sweepRangeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sweepRangeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    sweepRangeSlider.setRange(0.0, 2400.0);
    sweepRangeSlider.setTextValueSuffix(" cents");
    addAndMakeVisible(sweepRangeSlider);
    sweepRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "sweepRangeCents", sweepRangeSlider);
    
    sweepRangeLabel.setText("Range", juce::dontSendNotification);
    sweepRangeLabel.attachToComponent(&sweepRangeSlider, true);
    addAndMakeVisible(sweepRangeLabel);
    
    // Visual controls
    visualGroup.setText("Visual");
    addAndMakeVisible(visualGroup);
    
    mediumSelector.addItem("Plate", 1);
    mediumSelector.addItem("Membrane", 2);
    mediumSelector.addItem("Water", 3);
    addAndMakeVisible(mediumSelector);
    mediumAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "medium", mediumSelector);
    
    mediumLabel.setText("Medium", juce::dontSendNotification);
    mediumLabel.attachToComponent(&mediumSelector, true);
    addAndMakeVisible(mediumLabel);
    
    geometrySelector.addItem("Square", 1);
    geometrySelector.addItem("Circle", 2);
    addAndMakeVisible(geometrySelector);
    geomAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "geom", geometrySelector);
    
    geometryLabel.setText("Geometry", juce::dontSendNotification);
    geometryLabel.attachToComponent(&geometrySelector, true);
    addAndMakeVisible(geometryLabel);
    
    mountingSelector.addItem("Edge Clamped", 1);
    mountingSelector.addItem("Center Clamped", 2);
    addAndMakeVisible(mountingSelector);
    mountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "mount", mountingSelector);
    
    mountingLabel.setText("Mounting", juce::dontSendNotification);
    mountingLabel.attachToComponent(&mountingSelector, true);
    addAndMakeVisible(mountingLabel);
    
    accuracySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    accuracySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    accuracySlider.setRange(0.0, 1.0);
    addAndMakeVisible(accuracySlider);
    accuracyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "accuracy", accuracySlider);
    
    accuracyLabel.setText("Accuracy", juce::dontSendNotification);
    accuracyLabel.attachToComponent(&accuracySlider, true);
    addAndMakeVisible(accuracyLabel);
    
    nodeThresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    nodeThresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    nodeThresholdSlider.setRange(0.0, 0.1);
    addAndMakeVisible(nodeThresholdSlider);
    nodeEpsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "nodeEps", nodeThresholdSlider);
    
    nodeThresholdLabel.setText("Node Threshold", juce::dontSendNotification);
    nodeThresholdLabel.attachToComponent(&nodeThresholdSlider, true);
    addAndMakeVisible(nodeThresholdLabel);
    
    grainAmountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    grainAmountSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    grainAmountSlider.setRange(0.0, 1.0);
    addAndMakeVisible(grainAmountSlider);
    grainAmtAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "grainAmt", grainAmountSlider);
    
    grainAmountLabel.setText("Grain Amount", juce::dontSendNotification);
    grainAmountLabel.attachToComponent(&grainAmountSlider, true);
    addAndMakeVisible(grainAmountLabel);
    
    colorModeSelector.addItem("Mono", 1);
    colorModeSelector.addItem("Heat", 2);
    addAndMakeVisible(colorModeSelector);
    colorModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, "colorMode", colorModeSelector);
    
    colorModeLabel.setText("Color Mode", juce::dontSendNotification);
    colorModeLabel.attachToComponent(&colorModeSelector, true);
    addAndMakeVisible(colorModeLabel);
    
    // Action buttons
    resetAccumButton.setButtonText("Reset Accumulation");
    resetAccumButton.addListener(this);
    addAndMakeVisible(resetAccumButton);
    
    saveImageButton.setButtonText("Save Image");
    saveImageButton.addListener(this);
    addAndMakeVisible(saveImageButton);
    
    // Preset controls
    presetSelector.addItem("Default", 1);
    presetSelector.addListener(this);
    addAndMakeVisible(presetSelector);
    updatePresetList();
    
    presetLabel.setText("Presets", juce::dontSendNotification);
    presetLabel.attachToComponent(&presetSelector, true);
    addAndMakeVisible(presetLabel);
    
    savePresetButton.setButtonText("Save Preset");
    savePresetButton.addListener(this);
    addAndMakeVisible(savePresetButton);
    
    // About label
    aboutLabel.setText("GOE Cymaglyph — Cymatics-inspired visual tone generator", juce::dontSendNotification);
    aboutLabel.setJustificationType(juce::Justification::centred);
    aboutLabel.setFont(juce::Font(12.0f, juce::Font::italic));
    aboutLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(aboutLabel);
}

void GOECymaglyphAudioProcessorEditor::setupLayout()
{
    // Layout will be handled in resized()
}

void GOECymaglyphAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).darker(0.3f));
    
    // Draw a border around the visualizer
    g.setColour(juce::Colours::cyan.withAlpha(0.3f));
    g.drawRect(visualizer->getBounds(), 2);
}

void GOECymaglyphAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Footer
    auto footer = bounds.removeFromBottom(20);
    aboutLabel.setBounds(footer);
    
    // Top controls area
    auto controlsArea = bounds.removeFromTop(250);
    
    // Visualizer takes remaining space
    visualizer->setBounds(bounds.reduced(10));
    
    // Top row - main audio controls
    auto topRow = controlsArea.removeFromTop(100);
    auto audioControls = topRow.reduced(10);
    
    freqSlider.setBounds(audioControls.removeFromLeft(100));
    freqTextEntry.setBounds(audioControls.removeFromLeft(60).reduced(5, 35));
    gainSlider.setBounds(audioControls.removeFromLeft(80));
    waveformSelector.setBounds(audioControls.removeFromLeft(100).reduced(0, 30));
    gateButton.setBounds(audioControls.removeFromLeft(60).reduced(5, 35));
    
    // A4 Reference
    auto a4Area = audioControls.removeFromLeft(200);
    a4RefSlider.setBounds(a4Area.reduced(50, 35));
    
    // Preset controls
    auto presetArea = audioControls.removeFromRight(250);
    savePresetButton.setBounds(presetArea.removeFromRight(80).reduced(0, 35));
    presetSelector.setBounds(presetArea.reduced(50, 35));
    
    // Middle row - sweep controls
    auto middleRow = controlsArea.removeFromTop(75);
    sweepGroup.setBounds(middleRow.removeFromLeft(400).reduced(10));
    auto sweepArea = sweepGroup.getBounds().reduced(10, 20);
    sweepEnableButton.setBounds(sweepArea.removeFromLeft(60).reduced(0, 10));
    sweepRateSlider.setBounds(sweepArea.removeFromTop(25).reduced(40, 0));
    sweepRangeSlider.setBounds(sweepArea.removeFromTop(25).reduced(40, 0));
    
    // Bottom row - visual controls
    auto bottomRow = controlsArea.removeFromTop(75);
    visualGroup.setBounds(bottomRow.reduced(10));
    auto visualArea = visualGroup.getBounds().reduced(10, 20);
    
    auto visualRow1 = visualArea.removeFromTop(25);
    mediumSelector.setBounds(visualRow1.removeFromLeft(150).reduced(60, 0));
    geometrySelector.setBounds(visualRow1.removeFromLeft(150).reduced(60, 0));
    mountingSelector.setBounds(visualRow1.removeFromLeft(150).reduced(60, 0));
    colorModeSelector.setBounds(visualRow1.removeFromLeft(150).reduced(60, 0));
    
    auto visualRow2 = visualArea.removeFromTop(25);
    accuracySlider.setBounds(visualRow2.removeFromLeft(200).reduced(80, 0));
    nodeThresholdSlider.setBounds(visualRow2.removeFromLeft(200).reduced(80, 0));
    grainAmountSlider.setBounds(visualRow2.removeFromLeft(200).reduced(80, 0));
    
    resetAccumButton.setBounds(visualRow2.removeFromLeft(120).reduced(5));
    saveImageButton.setBounds(visualRow2.removeFromLeft(100).reduced(5));
}

void GOECymaglyphAudioProcessorEditor::timerCallback()
{
    // Update visualizer with current frequency
    visualizer->setFrequency(audioProcessor.getCurrentFrequency());
    
    // Update frequency text entry to match slider
    freqTextEntry.setText(juce::String(freqSlider.getValue(), 1), juce::dontSendNotification);
}

void GOECymaglyphAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    juce::ignoreUnused(slider);
}

void GOECymaglyphAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &resetAccumButton)
    {
        visualizer->resetAccumulation();
    }
    else if (button == &saveImageButton)
    {
        saveImage();
    }
    else if (button == &savePresetButton)
    {
        juce::AlertWindow w("Save Preset", "Enter preset name:", juce::AlertWindow::NoIcon);
        w.addTextEditor("name", "My Preset");
        w.addButton("Save", 1);
        w.addButton("Cancel", 0);
        
        if (w.runModalLoop() == 1)
        {
            auto presetName = w.getTextEditorContents("name");
            audioProcessor.savePreset(presetName);
            updatePresetList();
        }
    }
}

void GOECymaglyphAudioProcessorEditor::updatePresetList()
{
    presetSelector.clear();
    presetSelector.addItem("Default", 1);
    
    auto presets = audioProcessor.getPresetNames();
    for (int i = 0; i < presets.size(); ++i)
    {
        presetSelector.addItem(presets[i], i + 2);
    }
}

void GOECymaglyphAudioProcessorEditor::saveImage()
{
    juce::FileChooser chooser("Save Cymaglyph Image",
                             juce::File::getSpecialLocation(juce::File::userPicturesDirectory),
                             "*.png");
    
    if (chooser.browseForFileToSave(true))
    {
        auto file = chooser.getResult();
        visualizer->saveImage(file);
    }
}