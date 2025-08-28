#include "SynthEngine.h"
#include <juce_core/juce_core.h>

const std::array<SynthEngine::ModeInfo, SynthEngine::NumModes> SynthEngine::modeInfoTable = {{
    {"Pure Sine", "Clean sine waves", 
        juce::Colour(255, 105, 180), juce::Colour(135, 206, 250), juce::Colour(255, 182, 193)}, // Pink to Blue
    
    {"Warm Saw", "Analog-style sawtooth",
        juce::Colour(255, 140, 0), juce::Colour(255, 215, 0), juce::Colour(255, 69, 0)}, // Orange to Yellow
    
    {"Glass Triangle", "Crystalline triangle waves",
        juce::Colour(0, 255, 255), juce::Colour(240, 248, 255), juce::Colour(175, 238, 238)}, // Cyan to White
    
    {"Velvet Square", "Smooth filtered square",
        juce::Colour(128, 0, 128), juce::Colour(255, 215, 0), juce::Colour(238, 130, 238)}, // Purple to Gold
    
    {"Formant", "Vocal-like formants",
        juce::Colour(0, 255, 0), juce::Colour(0, 128, 128), juce::Colour(0, 255, 127)}, // Green to Teal
    
    {"Granular", "Grain synthesis texture",
        juce::Colour(255, 0, 0), juce::Colour(255, 140, 0), juce::Colour(255, 69, 0)}, // Red to Amber
    
    {"Harmonic", "Additive harmonics",
        juce::Colour(255, 0, 0), juce::Colour(0, 255, 0), juce::Colour(0, 0, 255)}, // Rainbow (RGB)
    
    {"Metallic", "FM bell-like tones",
        juce::Colour(192, 192, 192), juce::Colour(0, 191, 255), juce::Colour(224, 224, 224)}, // Silver to Blue
    
    {"Organic", "Natural filtered noise",
        juce::Colour(139, 69, 19), juce::Colour(34, 139, 34), juce::Colour(107, 142, 35)}, // Brown to Green
    
    {"Quantum", "Phase distortion synthesis",
        juce::Colour(255, 0, 255), juce::Colour(138, 43, 226), juce::Colour(255, 105, 180)} // Magenta to Violet
}};

SynthEngine::SynthEngine()
{
    // Initialize grain buffer with random values
    for (auto& grain : grainBuffer)
    {
        grain = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * 0.3f;
    }
}

float SynthEngine::generateSample(float phase, float frequency, int modeIndex)
{
    switch (modeIndex)
    {
        case PureSine:
            return generateSine(phase);
            
        case WarmSaw:
            return generateSaw(phase);
            
        case GlassTriangle:
            return generateTriangle(phase);
            
        case VelvetSquare:
            return generateSquare(phase) * 0.7f; // Softer square
            
        case Formant:
            return generateFormant(phase, frequency);
            
        case Granular:
            return generateGranular(phase, frequency);
            
        case Harmonic:
            return generateHarmonic(phase, frequency);
            
        case Metallic:
            return generateMetallic(phase, frequency);
            
        case Organic:
            return generateOrganic(phase, frequency);
            
        case Quantum:
            return generateQuantum(phase, frequency);
            
        default:
            return generateSine(phase);
    }
}

SynthEngine::ModeInfo SynthEngine::getModeInfo(int modeIndex)
{
    if (modeIndex >= 0 && modeIndex < NumModes)
        return modeInfoTable[modeIndex];
    return modeInfoTable[0];
}

juce::StringArray SynthEngine::getModeNames()
{
    juce::StringArray names;
    for (const auto& mode : modeInfoTable)
    {
        names.add(mode.name);
    }
    return names;
}

float SynthEngine::generateSine(float phase)
{
    return std::sin(2.0f * juce::MathConstants<float>::pi * phase);
}

float SynthEngine::generateSaw(float phase)
{
    // Warm analog-style saw with slight high-frequency rolloff
    float saw = 2.0f * phase - 1.0f;
    // Add slight even harmonics for warmth
    saw += 0.05f * std::sin(4.0f * juce::MathConstants<float>::pi * phase);
    return saw * 0.8f;
}

float SynthEngine::generateTriangle(float phase)
{
    // Crystalline triangle with subtle chorus effect
    float tri = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
    // Add shimmer
    tri += 0.02f * std::sin(8.0f * juce::MathConstants<float>::pi * phase);
    return tri;
}

float SynthEngine::generateSquare(float phase)
{
    // Band-limited square wave with velvet-smooth filtering
    float square = 0.0f;
    const float fundamental = 2.0f * juce::MathConstants<float>::pi * phase;
    
    // Only first 7 harmonics for smoother sound
    for (int harmonic = 1; harmonic <= 7; harmonic += 2)
    {
        square += std::sin(fundamental * harmonic) / harmonic;
    }
    
    return square * (4.0f / juce::MathConstants<float>::pi) * 0.7f;
}

float SynthEngine::generateFormant(float phase, float frequency)
{
    // Vocal formant synthesis
    float output = 0.0f;
    
    // Mix two formant frequencies
    float f1 = formants[0][0] / frequency;
    float f2 = formants[0][1] / frequency;
    
    // Generate formant peaks using resonant filters simulation
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * f1) * 0.6f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * f2) * 0.4f;
    
    // Add fundamental
    output += generateSine(phase) * 0.3f;
    
    return output * 0.5f;
}

float SynthEngine::generateGranular(float phase, float frequency)
{
    // Simple granular synthesis
    int index = static_cast<int>(phase * grainBuffer.size()) % grainBuffer.size();
    float grain = grainBuffer[index];
    
    // Modulate with envelope
    float envelope = std::sin(juce::MathConstants<float>::pi * phase);
    
    // Mix with fundamental
    return (grain * envelope + generateSine(phase) * 0.3f) * 0.6f;
}

float SynthEngine::generateHarmonic(float phase, float frequency)
{
    // Additive synthesis with multiple harmonics
    float output = 0.0f;
    const float fundamental = 2.0f * juce::MathConstants<float>::pi * phase;
    
    // Add harmonics with decreasing amplitude
    for (int h = 1; h <= 8; h++)
    {
        float harmonicAmp = 1.0f / (h * harmonicSpread);
        output += std::sin(fundamental * h) * harmonicAmp;
    }
    
    return output * 0.3f;
}

float SynthEngine::generateMetallic(float phase, float frequency)
{
    // FM synthesis for metallic/bell sounds
    modulatorPhase += modulatorRatio / 44100.0f; // Approximate sample rate
    if (modulatorPhase >= 1.0f) modulatorPhase -= 1.0f;
    
    float modulator = std::sin(2.0f * juce::MathConstants<float>::pi * modulatorPhase);
    float modulation = modulationIndex * modulator;
    
    // Apply FM
    float carrier = std::sin(2.0f * juce::MathConstants<float>::pi * (phase + modulation));
    
    return carrier * 0.5f;
}

float SynthEngine::generateOrganic(float phase, float frequency)
{
    // Filtered noise with resonance
    float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * 0.1f;
    
    // Mix with fundamental for pitched element
    float fundamental = generateSine(phase);
    
    // Simulate resonant filter effect
    float filtered = fundamental * 0.7f + noise * 0.3f;
    filtered += std::sin(4.0f * juce::MathConstants<float>::pi * phase) * resonance * 0.2f;
    
    return filtered * 0.6f;
}

float SynthEngine::generateQuantum(float phase, float frequency)
{
    // Phase distortion synthesis
    float distortedPhase = phase;
    
    // Apply phase distortion
    if (phase < 0.5f)
    {
        distortedPhase = phase * phase * 4.0f;
    }
    else
    {
        float p = (phase - 0.5f) * 2.0f;
        distortedPhase = 1.0f - (1.0f - p) * (1.0f - p);
    }
    
    // Generate waveform with distorted phase
    float output = std::sin(2.0f * juce::MathConstants<float>::pi * distortedPhase);
    
    // Add shimmering harmonics
    output += std::sin(6.0f * juce::MathConstants<float>::pi * phase) * 0.15f;
    
    return output * 0.6f;
}