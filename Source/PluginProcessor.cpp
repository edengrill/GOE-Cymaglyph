#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

SandWizardAudioProcessor::SandWizardAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Initialize synthesis engine with advanced modes
    synthEngine = std::make_unique<SynthEngine>();
    
    // Initialize smoothed values
    smoothedFreq.setCurrentAndTargetValue(440.0f);
    smoothedGain.setCurrentAndTargetValue(0.7f);
    
    // Initialize all voices
    for (auto& voice : voices)
    {
        voice.reset();
    }
}

SandWizardAudioProcessor::~SandWizardAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SandWizardAudioProcessor::createParameterLayout()
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

void SandWizardAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Not actively used in v2
}

bool SandWizardAudioProcessor::isPlaying() const
{
    // Check monophonic mode
    if (isMonophonic.load())
    {
        return currentMonoNote >= 0;
    }
    
    // Check polyphonic voices
    for (const auto& voice : voices)
    {
        if (voice.active && voice.amplitude > 0.01f)
            return true;
    }
    return false;
}

std::vector<float> SandWizardAudioProcessor::getActiveFrequencies() const
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

void SandWizardAudioProcessor::prepareToPlay(double sr, int samplesPerBlock)
{
    sampleRate = sr;
    
    // Configure smoothing
    smoothedFreq.reset(sr, 0.01); // 10ms smoothing
    smoothedGain.reset(sr, 0.01);
    
    // Set initial values
    smoothedFreq.setCurrentAndTargetValue(440.0f);
    smoothedGain.setCurrentAndTargetValue(0.5f);
}

void SandWizardAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SandWizardAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

void SandWizardAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
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
    
    // Clear buffer first
    buffer.clear();
    
    bool mono = isMonophonic.load();
    int synthMode = currentSynthMode.load();
    
    if (mono)
    {
        // Monophonic mode - single note at a time
        if (currentMonoNote < 0)
        {
            return; // No note playing
        }
        
        float freq = noteToFrequency(currentMonoNote);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float gain = smoothedGain.getNextValue();
            
            // Generate waveform using synthesis engine
            float output = synthEngine->generateSample(monoPhase, freq, synthMode) * gain;
            
            // Write to all channels
            for (int channel = 0; channel < numChannels; ++channel)
            {
                buffer.getWritePointer(channel)[sample] = output;
            }
            
            // Update phase
            const float phaseInc = freq * phaseIncBase;
            monoPhase += phaseInc;
            if (monoPhase >= 1.0f) monoPhase -= 1.0f;
        }
        
        currentFrequency.store(freq);
        currentPhase.store(monoPhase);
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
                        // Generate waveform using synthesis engine
                        output += synthEngine->generateSample(voice.phase, voice.frequency, synthMode) * voice.amplitude;
                        
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


void SandWizardAudioProcessor::handleMidiMessage(const juce::MidiMessage& message)
{
    bool mono = isMonophonic.load();
    
    if (mono)
    {
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            
            // Add to held notes list if not already there
            auto it = std::find(heldMonoNotes.begin(), heldMonoNotes.end(), noteNumber);
            if (it == heldMonoNotes.end())
            {
                heldMonoNotes.push_back(noteNumber);
            }
            
            // Play the new note
            currentMonoNote = noteNumber;
            float freq = noteToFrequency(currentMonoNote);
            currentFrequency.store(freq);
            smoothedFreq.setTargetValue(freq);
            // Don't reset synthesis engine for smoother transitions
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            
            // Remove from held notes
            auto it = std::find(heldMonoNotes.begin(), heldMonoNotes.end(), noteNumber);
            if (it != heldMonoNotes.end())
            {
                heldMonoNotes.erase(it);
            }
            
            // If this was the current note, switch to the most recent held note
            if (noteNumber == currentMonoNote)
            {
                if (!heldMonoNotes.empty())
                {
                    // Return to the most recently pressed note still held
                    currentMonoNote = heldMonoNotes.back();
                    float freq = noteToFrequency(currentMonoNote);
                    currentFrequency.store(freq);
                    smoothedFreq.setTargetValue(freq);
                }
                else
                {
                    // No more notes held
                    currentMonoNote = -1;
                }
            }
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            // Clear all held notes
            heldMonoNotes.clear();
            currentMonoNote = -1;
        }
    }
    else // Polyphonic
    {
        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            
            // First check if this note is already playing (prevent duplicates)
            Voice* existingVoice = findVoiceForNote(noteNumber);
            if (existingVoice != nullptr)
            {
                // Retrigger the existing voice instead of creating a new one
                existingVoice->phase = 0.0f;
                existingVoice->targetAmplitude = message.getFloatVelocity();
                return;
            }
            
            // Find a free voice or steal the oldest/quietest one
            Voice* voice = findFreeVoice();
            if (voice == nullptr)
            {
                // Steal the quietest voice
                voice = &voices[0];
                for (auto& v : voices)
                {
                    if (v.amplitude < voice->amplitude)
                    {
                        voice = &v;
                    }
                }
            }
            
            voice->active = true;
            voice->noteNumber = noteNumber;
            voice->frequency = noteToFrequency(noteNumber);
            voice->phase = 0.0f;
            voice->targetAmplitude = message.getFloatVelocity();
            voice->amplitude = 0.0f; // Start from 0 for smooth attack
        }
        else if (message.isNoteOff())
        {
            // Find ALL voices playing this note (in case of duplicates)
            int noteNumber = message.getNoteNumber();
            for (auto& voice : voices)
            {
                if (voice.active && voice.noteNumber == noteNumber)
                {
                    voice.targetAmplitude = 0.0f; // Start release
                }
            }
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            // Emergency stop all voices
            for (auto& voice : voices)
            {
                voice.reset();
            }
        }
    }
}

SandWizardAudioProcessor::Voice* SandWizardAudioProcessor::findFreeVoice()
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

SandWizardAudioProcessor::Voice* SandWizardAudioProcessor::findVoiceForNote(int noteNumber)
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

float SandWizardAudioProcessor::noteToFrequency(int noteNumber)
{
    // Apply octave shift (12 semitones per octave)
    int shiftedNote = noteNumber + (octaveShift.load() * 12);
    return a4Reference * std::pow(2.0f, (shiftedNote - 69) / 12.0f);
}

void SandWizardAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SandWizardAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
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

void SandWizardAudioProcessor::loadPreset(const juce::String& presetName)
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

void SandWizardAudioProcessor::savePreset(const juce::String& presetName)
{
    auto presetsDir = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
                        .getChildFile("Presets");
    presetsDir.createDirectory();
    
    auto presetFile = presetsDir.getChildFile(presetName + ".xml");
    
    juce::MemoryBlock data;
    getStateInformation(data);
    presetFile.replaceWithData(data.getData(), data.getSize());
}

juce::StringArray SandWizardAudioProcessor::getPresetNames()
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

juce::AudioProcessorEditor* SandWizardAudioProcessor::createEditor()
{
    return new SandWizardAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SandWizardAudioProcessor();
}