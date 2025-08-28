#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include <array>

class GOECymaglyphAudioProcessor : public juce::AudioProcessor,
                                   public juce::AudioProcessorValueTreeState::Listener
{
public:
    // Play modes
    enum PlayMode
    {
        Neutral = 0,
        Monophonic,
        Polyphonic
    };
    
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

    // Parameter listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    // APVTS getter
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Mode control
    PlayMode getCurrentMode() const { return currentMode; }
    void cycleMode();
    
    // Neutral mode controls
    void toggleGate();
    void adjustFrequency(float delta);
    
    // Get current playing state
    float getCurrentFrequency() const { return currentFrequency.load(); }
    float getCurrentPhase() const { return currentPhase.load(); }
    std::vector<float> getActiveFrequencies() const;
    
    // Preset management (keeping for potential future use)
    void loadPreset(const juce::String& presetName);
    void savePreset(const juce::String& presetName);
    juce::StringArray getPresetNames();

private:
    // Voice structure for polyphonic synthesis
    struct Voice
    {
        bool active = false;
        int noteNumber = -1;
        float frequency = 0.0f;
        float phase = 0.0f;
        float amplitude = 0.0f;
        float targetAmplitude = 0.0f;
        
        void reset()
        {
            active = false;
            noteNumber = -1;
            frequency = 0.0f;
            phase = 0.0f;
            amplitude = 0.0f;
            targetAmplitude = 0.0f;
        }
    };
    
    // Waveform generation
    float generateSine(float phase);
    float generateTriangle(float phase);
    float generateSquare(float phase);
    
    // MIDI handling
    void handleMidiMessage(const juce::MidiMessage& message);
    float noteToFrequency(int noteNumber);
    
    // Voice management
    Voice* findFreeVoice();
    Voice* findVoiceForNote(int noteNumber);
    
    // Parameters
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;
    
    // Mode state
    std::atomic<PlayMode> currentMode{Neutral};
    
    // Audio state
    double sampleRate = 44100.0;
    std::atomic<float> currentFrequency{440.0f};
    std::atomic<float> currentPhase{0.0f};
    float neutralPhase = 0.0f;
    std::atomic<bool> neutralGate{false};
    
    // Smoothing
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFreq;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;
    
    // Voices for polyphonic mode
    static constexpr int MAX_VOICES = 8;
    std::array<Voice, MAX_VOICES> voices;
    
    // Monophonic mode state
    int monoNoteNumber = -1;
    
    // A4 reference for MIDI
    float a4Reference = 440.0f;
    
    // Frequency adjustment state for neutral mode
    bool leftPressed = false;
    bool rightPressed = false;
    float frequencyAdjustSpeed = 0.0f;
    
    // Legacy sweep phase (not used in v2)
    float sweepPhase = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GOECymaglyphAudioProcessor)
};