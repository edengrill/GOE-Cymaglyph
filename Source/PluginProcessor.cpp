#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

GOECymaglyphAudioProcessor::GOECymaglyphAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Add parameter listeners
    apvts.addParameterListener("freq", this);
    apvts.addParameterListener("gain", this);
    apvts.addParameterListener("a4ref", this);
    
    // Initialize smoothed values
    smoothedFreq.setCurrentAndTargetValue(440.0f);
    smoothedGain.setCurrentAndTargetValue(0.2f);
}

GOECymaglyphAudioProcessor::~GOECymaglyphAudioProcessor()
{
    apvts.removeParameterListener("freq", this);
    apvts.removeParameterListener("gain", this);
    apvts.removeParameterListener("a4ref", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout GOECymaglyphAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Audio parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "freq", "Frequency", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 440.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain", 0.0f, 1.0f, 0.2f));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "wave", "Waveform", juce::StringArray{"Sine", "Triangle", "Square"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "gate", "Gate", true));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "a4ref", "A4 Reference", 415.0f, 466.0f, 440.0f));
    
    // Sweep parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "sweepOn", "Sweep Enable", false));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sweepRate", "Sweep Rate", 
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.5f), 0.5f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sweepRangeCents", "Sweep Range", 0.0f, 2400.0f, 200.0f));
    
    // Visual parameters
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "medium", "Medium", juce::StringArray{"Plate", "Membrane", "Water"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "geom", "Geometry", juce::StringArray{"Square", "Circle"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mount", "Mounting", juce::StringArray{"Edge Clamped", "Center Clamped"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "accuracy", "Accuracy", 0.0f, 1.0f, 0.5f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "nodeEps", "Node Threshold", 0.0f, 0.1f, 0.02f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainAmt", "Grain Amount", 0.0f, 1.0f, 0.7f));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "colorMode", "Color Mode", juce::StringArray{"Mono", "Heat"}, 0));
    
    return {params.begin(), params.end()};
}

void GOECymaglyphAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "freq")
    {
        smoothedFreq.setTargetValue(newValue);
    }
    else if (parameterID == "gain")
    {
        smoothedGain.setTargetValue(newValue);
    }
    else if (parameterID == "a4ref")
    {
        a4Reference = newValue;
    }
}

void GOECymaglyphAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    
    // Configure smoothing
    smoothedFreq.reset(sr, 0.01); // 10ms smoothing
    smoothedGain.reset(sr, 0.01);
    
    // Set initial values
    smoothedFreq.setCurrentAndTargetValue(apvts.getRawParameterValue("freq")->load());
    smoothedGain.setCurrentAndTargetValue(apvts.getRawParameterValue("gain")->load());
}

void GOECymaglyphAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GOECymaglyphAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

void GOECymaglyphAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Handle MIDI
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        handleMidiMessage(msg);
    }
    
    // Get parameters
    const bool gate = apvts.getRawParameterValue("gate")->load() > 0.5f;
    const int waveType = static_cast<int>(apvts.getRawParameterValue("wave")->load());
    const bool sweepOn = apvts.getRawParameterValue("sweepOn")->load() > 0.5f;
    const float sweepRate = apvts.getRawParameterValue("sweepRate")->load();
    const float sweepRangeCents = apvts.getRawParameterValue("sweepRangeCents")->load();
    
    if (!gate)
    {
        buffer.clear();
        return;
    }
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float phaseIncBase = static_cast<float>(1.0 / sampleRate);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Update smoothed values
        smoothedFreq.getNextValue();
        const float gain = smoothedGain.getNextValue();
        
        float freq = smoothedFreq.getCurrentValue();
        
        // Apply sweep if enabled
        if (sweepOn)
        {
            sweepPhase += sweepRate * phaseIncBase;
            if (sweepPhase >= 1.0f) sweepPhase -= 1.0f;
            
            const float sweepMod = std::sin(2.0f * juce::MathConstants<float>::pi * sweepPhase);
            const float cents = sweepRangeCents * sweepMod;
            freq *= std::pow(2.0f, cents / 1200.0f);
        }
        
        // Store for visualization
        currentFrequency.store(freq);
        currentPhase.store(phase);
        
        // Generate waveform
        float output = 0.0f;
        switch (waveType)
        {
            case 0: output = generateSine(phase); break;
            case 1: output = generateTriangle(phase); break;
            case 2: output = generateSquare(phase); break;
        }
        
        output *= gain;
        
        // Write to all channels
        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.getWritePointer(channel)[sample] = output;
        }
        
        // Update phase
        const float phaseInc = freq * phaseIncBase;
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

float GOECymaglyphAudioProcessor::generateSine(float ph)
{
    return std::sin(2.0f * juce::MathConstants<float>::pi * ph);
}

float GOECymaglyphAudioProcessor::generateTriangle(float ph)
{
    if (ph < 0.25f)
        return ph * 4.0f;
    else if (ph < 0.75f)
        return 2.0f - ph * 4.0f;
    else
        return ph * 4.0f - 4.0f;
}

float GOECymaglyphAudioProcessor::generateSquare(float ph)
{
    // Band-limited square using additive synthesis (first 10 harmonics)
    float output = 0.0f;
    const float fundamental = 2.0f * juce::MathConstants<float>::pi * ph;
    
    for (int harmonic = 1; harmonic <= 19; harmonic += 2)
    {
        output += std::sin(fundamental * harmonic) / harmonic;
    }
    
    return output * (4.0f / juce::MathConstants<float>::pi);
}

void GOECymaglyphAudioProcessor::handleMidiMessage(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        lastNoteNumber = message.getNoteNumber();
        const float freq = noteToFrequency(lastNoteNumber);
        apvts.getParameter("freq")->setValueNotifyingHost(
            apvts.getParameter("freq")->convertTo0to1(freq));
    }
    else if (message.isNoteOff() && message.getNoteNumber() == lastNoteNumber)
    {
        // Could implement note off behavior here if desired
    }
}

float GOECymaglyphAudioProcessor::noteToFrequency(int noteNumber)
{
    return a4Reference * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

void GOECymaglyphAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GOECymaglyphAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

void GOECymaglyphAudioProcessor::loadPreset(const juce::String& presetName)
{
    auto presetsDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                        .getChildFile("Presets");
    auto presetFile = presetsDir.getChildFile(presetName + ".xml");
    
    if (presetFile.existsAsFile())
    {
        juce::MemoryBlock data;
        presetFile.loadFileAsData(data);
        setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    }
}

void GOECymaglyphAudioProcessor::savePreset(const juce::String& presetName)
{
    auto presetsDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                        .getChildFile("Presets");
    presetsDir.createDirectory();
    
    auto presetFile = presetsDir.getChildFile(presetName + ".xml");
    
    juce::MemoryBlock data;
    getStateInformation(data);
    presetFile.replaceWithData(data.getData(), data.getSize());
}

juce::StringArray GOECymaglyphAudioProcessor::getPresetNames()
{
    juce::StringArray presets;
    auto presetsDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                        .getChildFile("Presets");
    
    if (presetsDir.exists())
    {
        for (auto& file : presetsDir.findChildFiles(juce::File::findFiles, false, "*.xml"))
        {
            presets.add(file.getFileNameWithoutExtension());
        }
    }
    
    return presets;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GOECymaglyphAudioProcessor();
}