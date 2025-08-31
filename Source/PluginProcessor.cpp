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
    
    // Oscillator Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "oscDetune", "Oscillator Detune", 
        juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f), 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "oscPhase", "Oscillator Phase", 0.0f, 1.0f, 0.0f));
    
    // Filter Parameters
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "filterType", "Filter Type", 
        juce::StringArray{"Lowpass", "Highpass", "Bandpass", "Notch", "Off"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterCutoff", "Filter Cutoff", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 1000.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterResonance", "Filter Resonance", 
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterDrive", "Filter Drive", 
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterEnvAmount", "Filter Env Amount", 
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));
    
    // ADSR Envelope Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ampAttack", "Amp Attack", 
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ampDecay", "Amp Decay", 
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ampSustain", "Amp Sustain", 0.0f, 1.0f, 0.7f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ampRelease", "Amp Release", 
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));
    
    // Filter Envelope Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterAttack", "Filter Attack", 
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterDecay", "Filter Decay", 
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.1f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterSustain", "Filter Sustain", 0.0f, 1.0f, 0.5f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filterRelease", "Filter Release", 
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.3f), 0.5f));
    
    // LFO Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lfo1Rate", "LFO 1 Rate", 
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 1.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lfo1Depth", "LFO 1 Depth", 0.0f, 1.0f, 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "lfo1Target", "LFO 1 Target", 
        juce::StringArray{"Off", "Pitch", "Filter", "Amplitude", "Pan"}, 0));
    
    // Effects Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbSize", "Reverb Size", 0.0f, 1.0f, 0.5f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "chorusMix", "Chorus Mix", 0.0f, 1.0f, 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "chorusRate", "Chorus Rate", 0.1f, 10.0f, 1.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "chorusDepth", "Chorus Depth", 0.0f, 1.0f, 0.3f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayTime", "Delay Time", 0.01f, 2.0f, 0.25f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.3f));
    
    // Global Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "masterVolume", "Master Volume", 
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 0.7f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "velocitySensitivity", "Velocity Sensitivity", 0.0f, 1.0f, 0.5f));
    
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        "voiceCount", "Voice Count", 1, 16, 8));
    
    // Keep visual parameters for backwards compatibility
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "medium", "Medium", juce::StringArray{"Plate", "Membrane", "Water"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "geom", "Geometry", juce::StringArray{"Square", "Circle"}, 0));
    
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "colorMode", "Color Mode", juce::StringArray{"Mono", "Heat"}, 0));
    
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
    
    // Configure smoothing with faster response for stability
    smoothedFreq.reset(sr, 0.005); // 5ms smoothing for quicker response
    smoothedGain.reset(sr, 0.005);
    
    // Set initial values
    smoothedFreq.setCurrentAndTargetValue(440.0f);
    smoothedGain.setCurrentAndTargetValue(0.5f);
    
    // Reset synthesis engine for clean start
    if (synthEngine)
    {
        synthEngine->reset();
    }
    
    // Clear all voices
    for (auto& voice : voices)
    {
        voice.reset();
    }
    
    // Clear mono state
    currentMonoNote = -1;
    monoPhase = 0.0f;
    heldMonoNotes.clear();
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
        
        float targetFreq = noteToFrequency(currentMonoNote);
        smoothedFreq.setTargetValue(targetFreq);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float gain = smoothedGain.getNextValue();
            const float freq = smoothedFreq.getNextValue(); // Smooth frequency changes
            
            // Generate waveform using synthesis engine
            float output = synthEngine->generateSample(monoPhase, freq, synthMode) * gain;
            
            // Apply DC blocker (high-pass filter at ~20Hz)
            const float dcBlockerCutoff = 0.995f;
            float dcBlockerOutput = output - dcBlockerX1 + dcBlockerCutoff * dcBlockerY1;
            dcBlockerX1 = output;
            dcBlockerY1 = dcBlockerOutput;
            output = dcBlockerOutput;
            
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
        
        currentFrequency.store(targetFreq);
        currentPhase.store(monoPhase);
    }
    else // Polyphonic
    {
        // Get envelope parameters
        float ampAttack = *apvts.getRawParameterValue("ampAttack");
        float ampDecay = *apvts.getRawParameterValue("ampDecay");
        float ampSustain = *apvts.getRawParameterValue("ampSustain");
        float ampRelease = *apvts.getRawParameterValue("ampRelease");
        
        float filterCutoff = *apvts.getRawParameterValue("filterCutoff");
        float filterResonance = *apvts.getRawParameterValue("filterResonance");
        int filterType = static_cast<int>(*apvts.getRawParameterValue("filterType"));
        float filterEnvAmount = *apvts.getRawParameterValue("filterEnvAmount");
        
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
                    
                    // Process amplitude envelope
                    processEnvelope(voice, ampAttack, ampDecay, ampSustain, ampRelease);
                    
                    if (voice.ampEnvLevel > 0.001f)
                    {
                        // Calculate filter cutoff with envelope modulation
                        float envModulatedCutoff = filterCutoff;
                        if (filterEnvAmount != 0.0f)
                        {
                            // Filter envelope processing (simplified for now)
                            envModulatedCutoff = filterCutoff * (1.0f + filterEnvAmount * voice.filterEnvLevel);
                            envModulatedCutoff = std::clamp(envModulatedCutoff, 20.0f, 20000.0f);
                        }
                        
                        // Generate waveform using synthesis engine
                        float voiceOut = synthEngine->generateSample(voice.phase, voice.frequency, synthMode);
                        
                        // Apply amplitude envelope and velocity
                        voiceOut *= voice.ampEnvLevel * voice.targetAmplitude;
                        
                        output += voiceOut;
                        
                        // Update phase
                        const float phaseInc = voice.frequency * phaseIncBase;
                        voice.phase += phaseInc;
                        if (voice.phase >= 1.0f) voice.phase -= 1.0f;
                    }
                    else if (voice.ampEnvStage == Voice::Off)
                    {
                        voice.reset(); // Voice has completed its envelope
                    }
                }
            }
            
            // Scale output by number of voices to prevent clipping
            if (activeVoices > 0)
            {
                output *= smoothedGain.getNextValue() / std::sqrt(static_cast<float>(activeVoices));
            }
            
            // Apply DC blocker
            const float dcBlockerCutoff = 0.995f;
            float dcBlockerOutput = output - dcBlockerX1 + dcBlockerCutoff * dcBlockerY1;
            dcBlockerX1 = output;
            dcBlockerY1 = dcBlockerOutput;
            output = dcBlockerOutput;
            
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
            
            // Remove note if it exists (to re-add at end for last-note priority)
            heldMonoNotes.erase(
                std::remove(heldMonoNotes.begin(), heldMonoNotes.end(), noteNumber),
                heldMonoNotes.end()
            );
            
            // Add note to end of list (most recent)
            heldMonoNotes.push_back(noteNumber);
            
            // Limit the size to prevent memory issues
            if (heldMonoNotes.size() > 10)
            {
                heldMonoNotes.erase(heldMonoNotes.begin());
            }
            
            // Always play the most recent note
            currentMonoNote = noteNumber;
            float freq = noteToFrequency(currentMonoNote);
            currentFrequency.store(freq);
            smoothedFreq.setTargetValue(freq);
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            
            // Remove from held notes
            heldMonoNotes.erase(
                std::remove(heldMonoNotes.begin(), heldMonoNotes.end(), noteNumber),
                heldMonoNotes.end()
            );
            
            // If this was the current note, play the most recent remaining note
            if (noteNumber == currentMonoNote)
            {
                if (!heldMonoNotes.empty())
                {
                    // Play the last note in the list (most recently pressed that's still held)
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
            float velocity = message.getFloatVelocity();
            
            // Check if this note is already playing
            Voice* existingVoice = findVoiceForNote(noteNumber);
            if (existingVoice != nullptr)
            {
                // Retrigger existing voice
                existingVoice->targetAmplitude = velocity;
                existingVoice->amplitude = existingVoice->amplitude * 0.5f; // Soft retrigger
                return;
            }
            
            // Find a free voice
            Voice* voice = findFreeVoice();
            if (voice == nullptr)
            {
                // Find the quietest voice to steal
                float minAmp = 1.0f;
                voice = nullptr;
                for (auto& v : voices)
                {
                    if (v.amplitude < minAmp)
                    {
                        minAmp = v.amplitude;
                        voice = &v;
                    }
                }
                
                // If we still don't have a voice, use the first one
                if (voice == nullptr)
                {
                    voice = &voices[0];
                }
            }
            
            // Initialize the voice
            voice->active = true;
            voice->noteNumber = noteNumber;
            voice->frequency = noteToFrequency(noteNumber);
            voice->phase = 0.0f;
            voice->targetAmplitude = velocity;
            voice->amplitude = 0.0f; // Start from 0 for smooth attack
            voice->startNote(); // Start envelopes
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            
            // Release all voices playing this note
            for (auto& voice : voices)
            {
                if (voice.active && voice.noteNumber == noteNumber)
                {
                    voice.stopNote(); // Trigger release stage
                    // Don't immediately deactivate, let envelope fade out
                }
            }
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            // Stop all voices immediately
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

void SandWizardAudioProcessor::processEnvelope(Voice& voice, float attack, float decay, float sustain, float release)
{
    const float sampleRate = getSampleRate();
    const float attackRate = 1.0f / (attack * sampleRate);
    const float decayRate = 1.0f / (decay * sampleRate);
    const float releaseRate = 1.0f / (release * sampleRate);
    
    // Process amplitude envelope
    switch (voice.ampEnvStage)
    {
        case Voice::Attack:
            voice.ampEnvLevel += attackRate;
            if (voice.ampEnvLevel >= 1.0f)
            {
                voice.ampEnvLevel = 1.0f;
                voice.ampEnvStage = Voice::Decay;
            }
            break;
            
        case Voice::Decay:
            voice.ampEnvLevel -= decayRate * (1.0f - sustain);
            if (voice.ampEnvLevel <= sustain)
            {
                voice.ampEnvLevel = sustain;
                voice.ampEnvStage = Voice::Sustain;
            }
            break;
            
        case Voice::Sustain:
            voice.ampEnvLevel = sustain;
            break;
            
        case Voice::Release:
            voice.ampEnvLevel -= releaseRate;
            if (voice.ampEnvLevel <= 0.0f)
            {
                voice.ampEnvLevel = 0.0f;
                voice.ampEnvStage = Voice::Off;
            }
            break;
            
        case Voice::Off:
            voice.ampEnvLevel = 0.0f;
            break;
    }
    
    // Process filter envelope (simplified - same as amp for now)
    voice.filterEnvLevel = voice.ampEnvLevel;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SandWizardAudioProcessor();
}