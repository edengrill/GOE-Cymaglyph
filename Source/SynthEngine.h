#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
#include <vector>
#include <random>

class SynthEngine
{
public:
    enum Mode
    {
        Crystalline = 0,
        SilkPad,
        VelvetKeys,
        LiquidBass,
        VintageBrass,
        CloudNine,
        GoldenLead,
        DreamPluck,
        AmbientWash,
        ProphetPoly,
        NumModes
    };
    
    struct ModeInfo
    {
        juce::String name;
        juce::String description;
        // Color palette - keeping exact same colors as before
        juce::Colour primaryColor;
        juce::Colour secondaryColor;
        juce::Colour accentColor;
    };
    
    SynthEngine();
    ~SynthEngine() = default;
    
    // Generate a sample for the current mode
    float generateSample(float phase, float frequency, int modeIndex);
    
    // Get mode information
    static ModeInfo getModeInfo(int modeIndex);
    static juce::StringArray getModeNames();
    
    // Reset internal state
    void reset();
    
    // Set velocity for expression
    void setVelocity(float vel) { velocity = vel; }
    
private:
    // Synthesis modes
    float generateCrystalline(float phase, float frequency);
    float generateSilkPad(float phase, float frequency);
    float generateVelvetKeys(float phase, float frequency);
    float generateLiquidBass(float phase, float frequency);
    float generateVintageBrass(float phase, float frequency);
    float generateCloudNine(float phase, float frequency);
    float generateGoldenLead(float phase, float frequency);
    float generateDreamPluck(float phase, float frequency);
    float generateAmbientWash(float phase, float frequency);
    float generateProphetPoly(float phase, float frequency);
    
    // Professional synthesis components
    struct Layer {
        float phase = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float detune = 0.0f;
        float pan = 0.0f;
    };
    
    struct Filter {
        // State variable filter
        float low = 0, band = 0, high = 0, notch = 0;
        float f = 0.1f, q = 1.0f;
        
        // Moog ladder filter state
        float stage[4] = {0, 0, 0, 0};
        float delay[4] = {0, 0, 0, 0};
        float feedback = 0.0f;
        
        void setStateVariable(float frequency, float resonance, float sampleRate);
        float processLowpass(float input);
        float processBandpass(float input);
        float processHighpass(float input);
        float processNotch(float input);
        
        void setMoogLadder(float frequency, float resonance, float sampleRate);
        float processMoogLadder(float input);
    };
    
    struct Envelope {
        float attack = 0.01f;
        float decay = 0.1f;
        float sustain = 0.7f;
        float release = 0.5f;
        float level = 0.0f;
        float state = 0.0f;
        
        float process(bool gate);
    };
    
    struct LFO {
        float phase = 0.0f;
        float frequency = 1.0f;
        float depth = 1.0f;
        
        float process();
    };
    
    struct Chorus {
        static constexpr int MAX_DELAY = 4096;
        float buffer[MAX_DELAY] = {0};
        int writeIndex = 0;
        float rate = 0.5f;
        float depth = 0.3f;
        float mix = 0.3f;
        float lfoPhase = 0.0f;
        
        float process(float input);
    };
    
    struct Reverb {
        static constexpr int NUM_COMBS = 8;
        static constexpr int NUM_ALLPASS = 4;
        
        struct CombFilter {
            std::vector<float> buffer;
            int index = 0;
            float feedback = 0.8f;
            float damp = 0.2f;
            float lastOut = 0.0f;
        };
        
        struct AllpassFilter {
            std::vector<float> buffer;
            int index = 0;
            float feedback = 0.5f;
        };
        
        CombFilter combs[NUM_COMBS];
        AllpassFilter allpasses[NUM_ALLPASS];
        float roomSize = 0.5f;
        float damping = 0.5f;
        float wetLevel = 0.3f;
        
        void initialize();
        float process(float input);
    };
    
    struct DelayLine {
        std::vector<float> buffer;
        int writeIndex = 0;
        float feedback = 0.4f;
        float time = 0.25f; // in seconds
        float mix = 0.2f;
        
        void resize(int size);
        float process(float input);
    };
    
    // Wavetable oscillator for high-quality waveforms
    struct WavetableOscillator {
        static constexpr int TABLE_SIZE = 2048;
        static constexpr int NUM_TABLES = 16;
        std::array<std::array<float, TABLE_SIZE>, NUM_TABLES> tables;
        float morphPosition = 0.0f;
        
        void initialize();
        float generate(float phase);
    };
    
    // FM operator for electric piano sounds
    struct FMOperator {
        float phase = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float feedback = 0.0f;
        float lastOutput = 0.0f;
        
        float generate(float modulation = 0.0f);
    };
    
    // Additive synthesis for brass
    struct Harmonic {
        float amplitude = 0.0f;
        float frequency = 1.0f;
        float phase = 0.0f;
    };
    
    // Granular engine
    struct Grain {
        float position = 0.0f;
        float duration = 0.1f;
        float pitch = 1.0f;
        float amplitude = 0.0f;
        float envelope = 0.0f;
        float pan = 0.0f;
        bool active = false;
    };
    
    // Physical modeling components
    struct KarplusStrong {
        std::vector<float> delayLine;
        int writeIndex = 0;
        float feedback = 0.99f;
        float damping = 0.5f;
        float lastSample = 0.0f;
        
        void setFrequency(float freq, float sampleRate);
        float process(float excitation);
    };
    
    // Synthesis state
    std::array<Layer, 4> layers;
    std::array<Filter, 4> filters;
    std::array<Envelope, 4> envelopes;
    std::array<LFO, 4> lfos;
    std::array<FMOperator, 6> fmOperators;
    std::array<Harmonic, 32> harmonics;
    std::array<Grain, 32> grains;
    std::array<KarplusStrong, 4> strings;
    
    WavetableOscillator wavetable;
    Chorus chorus;
    Reverb reverb;
    DelayLine delay;
    
    // Grain buffer for Cloud Nine
    std::vector<float> grainBuffer;
    int grainCounter = 0;
    
    // State tracking
    float velocity = 0.7f;
    float lastFrequency = 440.0f;
    float currentPhase = 0.0f;
    int sampleCounter = 0;
    
    // Random number generator
    std::mt19937 rng;
    std::uniform_real_distribution<float> randomDist;
    
    // Helper functions
    float softClip(float input);
    float hardClip(float input);
    float analogSaturate(float input);
    float randomFloat();
    void triggerGrain();
    void updateHarmonics(float frequency, int mode);
    float mixLayers(float dry, float wet, float mix);
    
    // Mode information table
    static const std::array<ModeInfo, NumModes> modeInfoTable;
};