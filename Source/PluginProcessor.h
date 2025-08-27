#pragma once

#include <JuceHeader.h>
#include <atomic>

class GOECymaglyphAudioProcessor : public juce::AudioProcessor,
                                   public juce::AudioProcessorValueTreeState::Listener
{
public:
    GOECymaglyphAudioProcessor();
    ~GOECymaglyphAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
    const juce::String getProgramName(int index) override { juce::ignoreUnused(index); return {}; }
    void changeProgramName(int index, const juce::String& newName) override { juce::ignoreUnused(index, newName); }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Visual parameters (thread-safe access)
    float getCurrentFrequency() const { return currentFrequency.load(); }
    float getCurrentPhase() const { return currentPhase.load(); }
    
    // Preset management
    void loadPreset(const juce::String& presetName);
    void savePreset(const juce::String& presetName);
    juce::StringArray getPresetNames();

private:
    // Parameter management
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    // Audio generation
    double sampleRate = 44100.0;
    float phase = 0.0f;
    float sweepPhase = 0.0f;
    std::atomic<float> currentFrequency{440.0f};
    std::atomic<float> currentPhase{0.0f};
    
    // Parameter smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFreq;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;
    
    // MIDI
    int lastNoteNumber = -1;
    float a4Reference = 440.0f;
    
    // Oscillator functions
    float generateSine(float phase);
    float generateTriangle(float phase);
    float generateSquare(float phase);
    
    // Helper functions
    void handleMidiMessage(const juce::MidiMessage& message);
    float noteToFrequency(int noteNumber);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GOECymaglyphAudioProcessor)
};