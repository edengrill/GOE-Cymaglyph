#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

GOECymaglyphAudioProcessor::GOECymaglyphAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize smoothed values
    smoothedFreq.setCurrentAndTargetValue(440.0f);
    smoothedGain.setCurrentAndTargetValue(0.5f);
    
    // Initialize all voices
    for (auto& voice : voices)
    {
        voice.reset();
    }
}

GOECymaglyphAudioProcessor::~GOECymaglyphAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout GOECymaglyphAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Keep minimal parameters for internal use
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "freq", "Frequency", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 440.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gain", "Gain", 0.0f, 1.0f, 0.5f));
    
    // Keep these for visual settings (can be changed via presets)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "medium", "Medium", juce::StringArray{"Plate", "Membrane", "Water"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "geom", "Geometry", juce::StringArray{"Square", "Circle"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "colorMode", "Color Mode", juce::StringArray{"Mono", "Heat"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "nodeEps", "Node Threshold", 0.0f, 0.1f, 0.02f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainAmt", "Grain Amount", 0.0f, 1.0f, 0.7f));
    
    return {params.begin(), params.end()};
}

void GOECymaglyphAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Not actively used in v2
}

void GOECymaglyphAudioProcessor::cycleMode()
{
    PlayMode mode = currentMode.load();
    
    // Reset voices when changing modes
    for (auto& voice : voices)
    {
        voice.reset();
    }
    monoNoteNumber = -1;
    
    // Cycle to next mode
    if (mode == Neutral)
    {
        currentMode = Monophonic;
    }
    else if (mode == Monophonic)
    {
        currentMode = Polyphonic;
    }
    else
    {
        currentMode = Neutral;
    }
}

void GOECymaglyphAudioProcessor::toggleGate()
{
    neutralGate = !neutralGate.load();
}

void GOECymaglyphAudioProcessor::adjustFrequency(float delta)
{
    float freq = currentFrequency.load();
    
    // Exponential frequency adjustment
    float factor = 1.0f + (delta * 0.02f);
    freq *= factor;
    
    // Clamp to valid range
    freq = juce::jlimit(20.0f, 20000.0f, freq);
    
    currentFrequency = freq;
    smoothedFreq.setTargetValue(freq);
}

std::vector<float> GOECymaglyphAudioProcessor::getActiveFrequencies() const
{
    std::vector<float> frequencies;
    
    for (const auto& voice : voices)
    {
        if (voice.active && voice.amplitude > 0.01f)
        {
            frequencies.push_back(voice.frequency);
        }
    }
    
    return frequencies;
}

void GOECymaglyphAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    
    // Configure smoothing
    smoothedFreq.reset(sr, 0.01); // 10ms smoothing
    smoothedGain.reset(sr, 0.01);
    
    // Set initial values
    smoothedFreq.setCurrentAndTargetValue(440.0f);
    smoothedGain.setCurrentAndTargetValue(0.5f);
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
    
    // Handle MIDI messages
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        handleMidiMessage(msg);
    }
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float phaseIncBase = static_cast<float>(1.0 / sampleRate);
    
    PlayMode mode = currentMode.load();
    
    // Clear buffer first
    buffer.clear();
    
    if (mode == Neutral)
    {
        // Neutral mode - manual frequency control
        if (!neutralGate.load())
        {
            return; // Gate is off
        }
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float freq = smoothedFreq.getNextValue();
            const float gain = smoothedGain.getNextValue();
            
            // Generate sine wave
            float output = generateSine(neutralPhase) * gain;
            
            // Write to all channels
            for (int channel = 0; channel < numChannels; ++channel)
            {
                buffer.getWritePointer(channel)[sample] = output;
            }
            
            // Update phase
            const float phaseInc = freq * phaseIncBase;
            neutralPhase += phaseInc;
            if (neutralPhase >= 1.0f) neutralPhase -= 1.0f;
        }
        
        currentPhase.store(neutralPhase);
    }
    else if (mode == Monophonic)
    {
        // Monophonic mode - single note at a time
        if (monoNoteNumber < 0)
        {
            return; // No note playing
        }
        
        float freq = noteToFrequency(monoNoteNumber);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float gain = smoothedGain.getNextValue();
            
            // Generate sine wave
            float output = generateSine(neutralPhase) * gain;
            
            // Write to all channels
            for (int channel = 0; channel < numChannels; ++channel)
            {
                buffer.getWritePointer(channel)[sample] = output;
            }
            
            // Update phase
            const float phaseInc = freq * phaseIncBase;
            neutralPhase += phaseInc;
            if (neutralPhase >= 1.0f) neutralPhase -= 1.0f;
        }
        
        currentFrequency.store(freq);
        currentPhase.store(neutralPhase);
    }
    else // Polyphonic
    {
        // Polyphonic mode - multiple voices
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float output = 0.0f;
            int activeVoices = 0;
            
            for (auto& voice : voices)
            {
                if (voice.active)
                {
                    activeVoices++;
                    
                    // Smooth amplitude changes
                    voice.amplitude = voice.amplitude * 0.99f + voice.targetAmplitude * 0.01f;
                    
                    if (voice.amplitude > 0.001f)
                    {
                        // Generate sine wave for this voice
                        output += generateSine(voice.phase) * voice.amplitude;
                        
                        // Update phase
                        const float phaseInc = voice.frequency * phaseIncBase;
                        voice.phase += phaseInc;
                        if (voice.phase >= 1.0f) voice.phase -= 1.0f;
                    }
                    else
                    {
                        voice.reset(); // Voice has faded out
                    }
                }
            }
            
            // Scale output by number of voices to prevent clipping
            if (activeVoices > 0)
            {
                output *= smoothedGain.getNextValue() / std::sqrt(static_cast<float>(activeVoices));
            }
            
            // Write to all channels
            for (int channel = 0; channel < numChannels; ++channel)
            {
                buffer.getWritePointer(channel)[sample] = output;
            }
        }
        
        // Update current frequency to average of playing notes for visualization
        float avgFreq = 0.0f;
        int count = 0;
        for (const auto& voice : voices)
        {
            if (voice.active && voice.amplitude > 0.01f)
            {
                avgFreq += voice.frequency;
                count++;
            }
        }
        if (count > 0)
        {
            currentFrequency.store(avgFreq / count);
        }
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
    // Band-limited square using additive synthesis
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
    PlayMode mode = currentMode.load();
    
    if (mode == Neutral)
    {
        // Ignore MIDI in neutral mode
        return;
    }
    else if (mode == Monophonic)
    {
        if (message.isNoteOn())
        {
            monoNoteNumber = message.getNoteNumber();
            float freq = noteToFrequency(monoNoteNumber);
            currentFrequency.store(freq);
            smoothedFreq.setTargetValue(freq);
        }
        else if (message.isNoteOff() && message.getNoteNumber() == monoNoteNumber)
        {
            monoNoteNumber = -1;
        }
    }
    else // Polyphonic
    {
        if (message.isNoteOn())
        {
            // Find a free voice or steal the oldest one
            Voice* voice = findFreeVoice();
            if (voice == nullptr)
            {
                // Steal first voice
                voice = &voices[0];
            }
            
            voice->active = true;
            voice->noteNumber = message.getNoteNumber();
            voice->frequency = noteToFrequency(voice->noteNumber);
            voice->phase = 0.0f;
            voice->targetAmplitude = message.getFloatVelocity();
            voice->amplitude = 0.0f; // Start from 0 for smooth attack
        }
        else if (message.isNoteOff())
        {
            // Find the voice playing this note
            Voice* voice = findVoiceForNote(message.getNoteNumber());
            if (voice != nullptr)
            {
                voice->targetAmplitude = 0.0f; // Start release
            }
        }
    }
}

GOECymaglyphAudioProcessor::Voice* GOECymaglyphAudioProcessor::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice.active)
        {
            return &voice;
        }
    }
    return nullptr;
}

GOECymaglyphAudioProcessor::Voice* GOECymaglyphAudioProcessor::findVoiceForNote(int noteNumber)
{
    for (auto& voice : voices)
    {
        if (voice.active && voice.noteNumber == noteNumber)
        {
            return &voice;
        }
    }
    return nullptr;
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

juce::AudioProcessorEditor* GOECymaglyphAudioProcessor::createEditor()
{
    return new GOECymaglyphAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GOECymaglyphAudioProcessor();
}