#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>

class SynthEngine
{
public:
    enum Mode
    {
        PureSine = 0,
        WarmSaw,
        GlassTriangle,
        VelvetSquare,
        Formant,
        Granular,
        Harmonic,
        Metallic,
        Organic,
        Quantum,
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
    
    // Synthesis parameters per mode
    void setFilterCutoff(float cutoff) { filterCutoff = cutoff; }
    void setResonance(float res) { resonance = res; }
    void setHarmonicSpread(float spread) { harmonicSpread = spread; }
    
private:
    // Basic waveforms
    float generateSine(float phase);
    float generateSaw(float phase);
    float generateTriangle(float phase);
    float generateSquare(float phase);
    
    // Advanced synthesis
    float generateFormant(float phase, float frequency);
    float generateGranular(float phase, float frequency);
    float generateHarmonic(float phase, float frequency);
    float generateMetallic(float phase, float frequency);
    float generateOrganic(float phase, float frequency);
    float generateQuantum(float phase, float frequency);
    
    // Synthesis parameters
    float filterCutoff = 1000.0f;
    float resonance = 1.0f;
    float harmonicSpread = 1.0f;
    
    // FM synthesis state
    float modulatorPhase = 0.0f;
    float modulatorRatio = 1.5f;
    float modulationIndex = 2.0f;
    
    // Granular synthesis
    std::array<float, 512> grainBuffer;
    int grainPosition = 0;
    
    // Formant frequencies
    static constexpr float formants[3][2] = {
        {700.0f, 1220.0f},   // A
        {300.0f, 2700.0f},   // I  
        {400.0f, 2000.0f}    // U
    };
    
    // Mode information table
    static const std::array<ModeInfo, NumModes> modeInfoTable;
};