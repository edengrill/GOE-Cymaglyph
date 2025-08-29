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
        AnalogBeast,
        Resonator,
        Morpheus,
        Vox,
        Texture,
        Spectral,
        DX7,
        Living,
        Nebula,
        NumModes
    };
    
    struct ModeInfo
    {
        juce::String name;
        juce::String description;
        // Color palette
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
    
private:
    // Advanced synthesis modes
    float generateCrystalline(float phase, float frequency);
    float generateAnalogBeast(float phase, float frequency);
    float generateResonator(float phase, float frequency);
    float generateMorpheus(float phase, float frequency);
    float generateVox(float phase, float frequency);
    float generateTexture(float phase, float frequency);
    float generateSpectral(float phase, float frequency);
    float generateDX7(float phase, float frequency);
    float generateLiving(float phase, float frequency);
    float generateNebula(float phase, float frequency);
    
    // Helper synthesis components
    struct WavetableOscillator {
        static constexpr int TABLE_SIZE = 512;
        static constexpr int NUM_TABLES = 8;
        std::array<std::array<float, TABLE_SIZE>, NUM_TABLES> tables;
        float morphPosition = 0.0f;
        
        void initialize();
        float generate(float phase);
    };
    
    struct DelayLine {
        std::vector<float> buffer;
        int writeIndex = 0;
        float feedback = 0.995f;
        
        void resize(int size);
        float process(float input);
    };
    
    struct StateVariableFilter {
        float low = 0, band = 0, high = 0;
        float f = 0.1f, q = 1.0f;
        
        void setParams(float frequency, float resonance, float sampleRate);
        float processLowpass(float input);
        float processBandpass(float input);
        float processHighpass(float input);
    };
    
    struct FMOperator {
        float phase = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        
        float generate(float modulation = 0.0f);
    };
    
    struct Grain {
        float position = 0.0f;
        float duration = 0.1f;
        float pitch = 1.0f;
        float amplitude = 0.0f;
        float envelope = 0.0f;
        bool active = false;
    };
    
    // Synthesis components
    WavetableOscillator wavetable;
    std::array<DelayLine, 4> delayLines;
    std::array<StateVariableFilter, 5> filters;
    std::array<FMOperator, 6> fmOperators;
    std::array<Grain, 16> grains;
    std::vector<float> grainBuffer;
    
    // Virtual analog components
    float analogDrift = 0.0f;
    float analogPhase2 = 0.0f;
    float analogPhase3 = 0.0f;
    float subPhase = 0.0f;
    
    // Chaos synthesis state
    float chaosX = 0.1f;
    float chaosY = 0.0f;
    float chaosZ = 0.0f;
    
    // Formant frequencies for vowels
    static constexpr float formantFreqs[5][5] = {
        {730, 1090, 2440, 3400, 4200},  // A
        {270, 2290, 3010, 3500, 4200},  // E
        {300, 870, 2240, 3100, 4200},   // I
        {570, 840, 2410, 3400, 4200},   // O
        {440, 1020, 2240, 2900, 4200}   // U
    };
    
    // Random number generator
    std::mt19937 rng;
    std::uniform_real_distribution<float> randomDist;
    
    // Internal state
    float lastFrequency = 440.0f;
    int grainCounter = 0;
    float morphX = 0.0f;
    float morphY = 0.0f;
    int currentVowel = 0;
    
    // Helper functions
    float softClip(float input);
    float randomFloat();
    void updateMorphPosition(float frequency);
    void triggerGrain();
    
    // Mode information table
    static const std::array<ModeInfo, NumModes> modeInfoTable;
};