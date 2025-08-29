#include "SynthEngine.h"
#include <algorithm>

// Keep the exact same color schemes as before
const std::array<SynthEngine::ModeInfo, SynthEngine::NumModes> SynthEngine::modeInfoTable = {{
    {"Crystalline", "Glass harmonics", 
        juce::Colour(255, 105, 180), juce::Colour(135, 206, 250), juce::Colour(255, 182, 193)}, // Pink to Blue
    
    {"Analog Beast", "Vintage warmth",
        juce::Colour(255, 140, 0), juce::Colour(255, 215, 0), juce::Colour(255, 69, 0)}, // Orange to Yellow
    
    {"Resonator", "Physical strings",
        juce::Colour(0, 255, 255), juce::Colour(240, 248, 255), juce::Colour(175, 238, 238)}, // Cyan to White
    
    {"Morpheus", "Evolving textures",
        juce::Colour(128, 0, 128), juce::Colour(255, 215, 0), juce::Colour(238, 130, 238)}, // Purple to Gold
    
    {"Vox", "Human vocals",
        juce::Colour(0, 255, 0), juce::Colour(0, 128, 128), juce::Colour(0, 255, 127)}, // Green to Teal
    
    {"Texture", "Grain clouds",
        juce::Colour(255, 0, 0), juce::Colour(255, 140, 0), juce::Colour(255, 69, 0)}, // Red to Amber
    
    {"Spectral", "Harmonic organ",
        juce::Colour(255, 0, 0), juce::Colour(0, 255, 0), juce::Colour(0, 0, 255)}, // Rainbow (RGB)
    
    {"DX7", "FM synthesis",
        juce::Colour(192, 192, 192), juce::Colour(0, 191, 255), juce::Colour(224, 224, 224)}, // Silver to Blue
    
    {"Living", "Chaos generator",
        juce::Colour(139, 69, 19), juce::Colour(34, 139, 34), juce::Colour(107, 142, 35)}, // Brown to Green
    
    {"Nebula", "Space atmosphere",
        juce::Colour(255, 0, 255), juce::Colour(138, 43, 226), juce::Colour(255, 105, 180)} // Magenta to Violet
}};

SynthEngine::SynthEngine() : rng(std::random_device{}()), randomDist(-1.0f, 1.0f)
{
    // Initialize wavetable oscillator with complex waveforms
    wavetable.initialize();
    
    // Initialize grain buffer with interesting texture
    grainBuffer.resize(4096);
    for (size_t i = 0; i < grainBuffer.size(); i++)
    {
        float t = static_cast<float>(i) / grainBuffer.size();
        // Create a complex texture with multiple harmonics
        grainBuffer[i] = std::sin(2.0f * juce::MathConstants<float>::pi * t * 3.0f) * 0.3f +
                         std::sin(2.0f * juce::MathConstants<float>::pi * t * 7.0f) * 0.2f +
                         std::sin(2.0f * juce::MathConstants<float>::pi * t * 11.0f) * 0.1f;
        grainBuffer[i] *= std::exp(-t * 2.0f); // Apply envelope
    }
    
    // Initialize delay lines for physical modeling
    for (auto& delay : delayLines)
    {
        delay.resize(2048);
    }
    
    // Initialize FM operators with different ratios
    float ratios[] = {1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 7.0f};
    for (size_t i = 0; i < fmOperators.size(); i++)
    {
        fmOperators[i].frequency = ratios[i];
        fmOperators[i].amplitude = 1.0f / (i + 1);
    }
}

void SynthEngine::reset()
{
    // Reset all phases and states
    for (auto& op : fmOperators)
        op.phase = 0.0f;
    
    for (auto& grain : grains)
        grain.active = false;
    
    analogPhase2 = 0.0f;
    analogPhase3 = 0.0f;
    subPhase = 0.0f;
    
    chaosX = 0.1f;
    chaosY = 0.0f;
    chaosZ = 0.0f;
}

float SynthEngine::generateSample(float phase, float frequency, int modeIndex)
{
    // Update internal frequency tracking
    if (std::abs(frequency - lastFrequency) > 0.1f)
    {
        lastFrequency = frequency;
        updateMorphPosition(frequency);
    }
    
    switch (modeIndex)
    {
        case Crystalline:    return generateCrystalline(phase, frequency);
        case AnalogBeast:    return generateAnalogBeast(phase, frequency);
        case Resonator:      return generateResonator(phase, frequency);
        case Morpheus:       return generateMorpheus(phase, frequency);
        case Vox:            return generateVox(phase, frequency);
        case Texture:        return generateTexture(phase, frequency);
        case Spectral:       return generateSpectral(phase, frequency);
        case DX7:            return generateDX7(phase, frequency);
        case Living:         return generateLiving(phase, frequency);
        case Nebula:         return generateNebula(phase, frequency);
        default:             return 0.0f;
    }
}

float SynthEngine::generateCrystalline(float phase, float frequency)
{
    // Wavetable synthesis with glass-like harmonics
    float output = wavetable.generate(phase);
    
    // Add inharmonic partials for bell-like quality
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 2.76f) * 0.15f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 5.4f) * 0.1f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 8.93f) * 0.05f;
    
    // Apply comb filtering for shimmer
    float delayed = delayLines[0].process(output * 0.3f);
    output = output * 0.7f + delayed;
    
    // Subtle ring modulation
    float ringMod = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 13.0f);
    output = output * (1.0f + ringMod * 0.1f);
    
    return softClip(output * 0.5f);
}

float SynthEngine::generateAnalogBeast(float phase, float frequency)
{
    // Virtual analog with multiple detuned oscillators
    const float detune1 = 0.997f;
    const float detune2 = 1.003f;
    
    // Update analog drift
    analogDrift += randomFloat() * 0.0001f;
    analogDrift *= 0.999f;
    
    // Main oscillator - supersaw
    float saw1 = 2.0f * phase - 1.0f;
    
    // Detuned oscillators
    analogPhase2 += (frequency * detune1) / 44100.0f;
    if (analogPhase2 >= 1.0f) analogPhase2 -= 1.0f;
    float saw2 = 2.0f * analogPhase2 - 1.0f;
    
    analogPhase3 += (frequency * detune2) / 44100.0f;
    if (analogPhase3 >= 1.0f) analogPhase3 -= 1.0f;
    float saw3 = 2.0f * analogPhase3 - 1.0f;
    
    // Sub oscillator (one octave down)
    subPhase += (frequency * 0.5f) / 44100.0f;
    if (subPhase >= 1.0f) subPhase -= 1.0f;
    float sub = std::sin(2.0f * juce::MathConstants<float>::pi * subPhase);
    
    // Mix oscillators
    float output = (saw1 + saw2 * 0.7f + saw3 * 0.7f) * 0.3f + sub * 0.4f;
    
    // Apply analog-style filter
    filters[0].setParams(2000.0f + frequency * 2.0f, 2.0f, 44100.0f);
    output = filters[0].processLowpass(output);
    
    // Add analog warmth (subtle saturation)
    output = std::tanh(output * 1.5f + analogDrift);
    
    return output * 0.6f;
}

float SynthEngine::generateResonator(float phase, float frequency)
{
    // Physical modeling - Karplus-Strong algorithm
    
    // Excitation signal (only at start of note)
    float excitation = 0.0f;
    if (phase < 0.01f)
    {
        excitation = randomFloat() * 0.5f;
    }
    
    // Set delay line length based on frequency
    int delayLength = static_cast<int>(44100.0f / frequency);
    if (delayLength != delayLines[1].buffer.size())
    {
        delayLines[1].resize(delayLength);
    }
    
    // Process through delay line with feedback
    float delayed = delayLines[1].process(excitation);
    
    // Apply damping filter (averaging)
    static float lastSample = 0.0f;
    float filtered = (delayed + lastSample) * 0.5f * 0.995f;
    lastSample = delayed;
    
    // Add sympathetic resonance
    float sympathetic = delayLines[2].process(filtered * 0.2f);
    
    // Mix with harmonics for richness
    float output = filtered + sympathetic * 0.3f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 2.0f) * 0.1f;
    
    return softClip(output * 0.7f);
}

float SynthEngine::generateMorpheus(float phase, float frequency)
{
    // Vector synthesis - 4-way morphing
    
    // Generate 4 different waveforms
    float sine = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    float saw = 2.0f * phase - 1.0f;
    float square = phase < 0.5f ? 1.0f : -1.0f;
    float noise = randomFloat() * 0.3f;
    
    // Morph position based on frequency
    updateMorphPosition(frequency);
    
    // Bilinear interpolation
    float top = sine * (1.0f - morphX) + saw * morphX;
    float bottom = square * (1.0f - morphX) + noise * morphX;
    float output = top * (1.0f - morphY) + bottom * morphY;
    
    // Apply formant filter for character
    filters[1].setParams(frequency * 3.0f, 5.0f, 44100.0f);
    output = filters[1].processBandpass(output);
    
    // Add movement with LFO
    float lfo = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.1f);
    output *= (1.0f + lfo * 0.2f);
    
    return output * 0.5f;
}

float SynthEngine::generateVox(float phase, float frequency)
{
    // Advanced formant synthesis
    
    // Glottal pulse excitation
    float excitation = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    excitation = excitation * excitation * excitation; // Shape the pulse
    
    // Cycle through vowels based on frequency
    currentVowel = static_cast<int>((frequency / 100.0f)) % 5;
    
    float output = 0.0f;
    
    // Apply formant filters
    for (int i = 0; i < 5; i++)
    {
        filters[i].setParams(formantFreqs[currentVowel][i], 10.0f + i * 5.0f, 44100.0f);
        float formant = filters[i].processBandpass(excitation);
        output += formant * (1.0f / (i + 1)); // Decreasing amplitude for higher formants
    }
    
    // Add breathiness
    float breath = randomFloat() * 0.05f;
    output = output * 0.9f + breath * 0.1f;
    
    // Vibrato
    float vibrato = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 5.0f) * 0.02f;
    output *= (1.0f + vibrato);
    
    return softClip(output * 0.4f);
}

float SynthEngine::generateTexture(float phase, float frequency)
{
    // Enhanced granular synthesis
    
    // Trigger new grains periodically
    grainCounter++;
    if (grainCounter > static_cast<int>(44100.0f / (frequency * 0.1f)))
    {
        grainCounter = 0;
        triggerGrain();
    }
    
    float output = 0.0f;
    
    // Process active grains
    for (auto& grain : grains)
    {
        if (grain.active)
        {
            // Calculate grain envelope (Hann window)
            float env = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * grain.envelope));
            
            // Read from grain buffer with pitch shift
            int bufferIndex = static_cast<int>(grain.position * grainBuffer.size()) % grainBuffer.size();
            float sample = grainBuffer[bufferIndex];
            
            // Apply envelope and amplitude
            output += sample * env * grain.amplitude;
            
            // Update grain state
            grain.envelope += 1.0f / (grain.duration * 44100.0f);
            grain.position += grain.pitch / 44100.0f;
            
            // Deactivate finished grains
            if (grain.envelope >= 1.0f)
            {
                grain.active = false;
            }
        }
    }
    
    // Add texture with filtered noise
    float texture = randomFloat() * 0.1f;
    filters[3].setParams(frequency * 4.0f, 3.0f, 44100.0f);
    texture = filters[3].processBandpass(texture);
    
    output = output * 0.7f + texture * 0.3f;
    
    // Spatial effect with delays
    float delayed = delayLines[3].process(output * 0.4f);
    
    return softClip((output + delayed) * 0.5f);
}

float SynthEngine::generateSpectral(float phase, float frequency)
{
    // Advanced additive synthesis with spectral control
    float output = 0.0f;
    const int numHarmonics = 32;
    
    // Generate harmonics with spectral envelope
    for (int h = 1; h <= numHarmonics; h++)
    {
        // Spectral envelope - emphasize certain harmonics
        float amplitude = 1.0f / h;
        
        // Create formant-like peaks
        if (h % 3 == 0) amplitude *= 2.0f;
        if (h % 7 == 0) amplitude *= 1.5f;
        
        // Frequency-dependent filtering
        float cutoff = 20.0f - (frequency / 100.0f);
        if (h > cutoff) amplitude *= std::exp(-(h - cutoff) * 0.2f);
        
        // Add harmonic with slight detuning for richness
        float detune = 1.0f + (h * 0.001f * std::sin(phase * h));
        output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * h * detune) * amplitude;
    }
    
    // Normalize and add movement
    output *= 0.1f;
    
    // Spectral shifting effect
    float shift = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.3f) * 0.1f;
    output *= (1.0f + shift);
    
    return softClip(output * 0.6f);
}

float SynthEngine::generateDX7(float phase, float frequency)
{
    // 6-operator FM synthesis
    
    // Update operator frequencies
    for (size_t i = 0; i < fmOperators.size(); i++)
    {
        fmOperators[i].phase += (frequency * fmOperators[i].frequency) / 44100.0f;
        if (fmOperators[i].phase >= 1.0f) fmOperators[i].phase -= 1.0f;
    }
    
    // Algorithm 5: Classic electric piano
    // 6->5->4, 3->2->1, then 1+4 to output
    float mod6 = fmOperators[5].generate(0.0f) * 2.0f;
    float mod5 = fmOperators[4].generate(mod6) * 1.5f;
    float carrier1 = fmOperators[3].generate(mod5);
    
    float mod3 = fmOperators[2].generate(0.0f) * 3.0f;
    float mod2 = fmOperators[1].generate(mod3) * 2.0f;
    float carrier2 = fmOperators[0].generate(mod2);
    
    float output = (carrier1 + carrier2) * 0.5f;
    
    // Add metallic character
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 7.13f) * 0.05f;
    
    // Dynamic brightness based on frequency
    float brightness = frequency / 1000.0f;
    output = output * (1.0f - brightness * 0.3f) + 
             std::tanh(output * 3.0f) * brightness * 0.3f;
    
    return output * 0.5f;
}

float SynthEngine::generateLiving(float phase, float frequency)
{
    // Chaos synthesis using Lorenz attractor
    
    const float dt = 0.01f;
    const float sigma = 10.0f;
    const float rho = 28.0f;
    const float beta = 8.0f / 3.0f;
    
    // Update Lorenz system
    float dx = sigma * (chaosY - chaosX);
    float dy = chaosX * (rho - chaosZ) - chaosY;
    float dz = chaosX * chaosY - beta * chaosZ;
    
    chaosX += dx * dt;
    chaosY += dy * dt;
    chaosZ += dz * dt;
    
    // Keep bounded
    chaosX = std::tanh(chaosX * 0.1f);
    chaosY = std::tanh(chaosY * 0.1f);
    chaosZ = std::tanh(chaosZ * 0.1f);
    
    // Use chaos to modulate synthesis
    float carrier = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    float modulation = chaosX * 0.5f;
    
    float output = carrier * (1.0f + modulation);
    
    // Add organic texture
    output += chaosY * 0.2f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * (2.0f + chaosZ)) * 0.3f;
    
    // Breathing effect
    float breath = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.2f);
    output *= (1.0f + breath * 0.3f);
    
    // Organic filtering
    filters[4].setParams(500.0f + chaosX * 2000.0f, 1.0f + std::abs(chaosY) * 5.0f, 44100.0f);
    output = filters[4].processLowpass(output);
    
    return softClip(output * 0.5f);
}

float SynthEngine::generateNebula(float phase, float frequency)
{
    // Hybrid synthesis - combining multiple techniques
    
    // Layer 1: Wavetable foundation
    float wavetableOut = wavetable.generate(phase) * 0.3f;
    
    // Layer 2: Granular texture
    float granularOut = 0.0f;
    for (const auto& grain : grains)
    {
        if (grain.active)
        {
            int idx = static_cast<int>(grain.position * grainBuffer.size()) % grainBuffer.size();
            granularOut += grainBuffer[idx] * grain.amplitude * 0.1f;
        }
    }
    
    // Layer 3: FM for metallic shimmer
    float fmMod = fmOperators[0].generate(0.0f) * 5.0f;
    float fmOut = std::sin(2.0f * juce::MathConstants<float>::pi * phase * (1.0f + fmMod)) * 0.2f;
    
    // Layer 4: Filtered noise for atmosphere
    float noise = randomFloat() * 0.1f;
    filters[2].setParams(frequency * 8.0f, 4.0f, 44100.0f);
    float filteredNoise = filters[2].processBandpass(noise);
    
    // Combine layers with frequency-dependent mixing
    float mixFactor = frequency / 1000.0f;
    float output = wavetableOut * (1.0f - mixFactor * 0.5f) +
                   granularOut * 0.5f +
                   fmOut * mixFactor +
                   filteredNoise * 0.3f;
    
    // Space reverb simulation with delays
    float reverb = 0.0f;
    for (int i = 0; i < 4; i++)
    {
        reverb += delayLines[i].process(output * 0.2f) * 0.25f;
    }
    
    output = output * 0.7f + reverb * 0.3f;
    
    // Add ethereal movement
    float lfo1 = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.13f);
    float lfo2 = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.37f);
    output *= (1.0f + lfo1 * 0.1f + lfo2 * 0.05f);
    
    return softClip(output * 0.6f);
}

// Helper function implementations

void SynthEngine::WavetableOscillator::initialize()
{
    // Create complex wavetables with different harmonic content
    for (int table = 0; table < NUM_TABLES; table++)
    {
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            float phase = static_cast<float>(i) / TABLE_SIZE;
            tables[table][i] = 0.0f;
            
            // Add harmonics with varying complexity
            int numHarmonics = 1 + table * 2;
            for (int h = 1; h <= numHarmonics; h++)
            {
                float amplitude = 1.0f / (h + table * 0.5f);
                // Add slight inharmonicity for metallic character
                float frequency = h * (1.0f + table * 0.01f * (h - 1));
                tables[table][i] += std::sin(2.0f * juce::MathConstants<float>::pi * phase * frequency) * amplitude;
            }
            
            // Normalize
            tables[table][i] *= 0.5f / numHarmonics;
        }
    }
}

float SynthEngine::WavetableOscillator::generate(float phase)
{
    // Find surrounding tables for morphing
    int tableA = static_cast<int>(morphPosition);
    int tableB = (tableA + 1) % NUM_TABLES;
    float blend = morphPosition - tableA;
    
    // Calculate index with interpolation
    float floatIndex = phase * TABLE_SIZE;
    int index = static_cast<int>(floatIndex) % TABLE_SIZE;
    int nextIndex = (index + 1) % TABLE_SIZE;
    float frac = floatIndex - index;
    
    // Interpolate within tables
    float sampleA = tables[tableA][index] * (1.0f - frac) + tables[tableA][nextIndex] * frac;
    float sampleB = tables[tableB][index] * (1.0f - frac) + tables[tableB][nextIndex] * frac;
    
    // Morph between tables
    return sampleA * (1.0f - blend) + sampleB * blend;
}

void SynthEngine::DelayLine::resize(int size)
{
    buffer.resize(size, 0.0f);
    writeIndex = 0;
}

float SynthEngine::DelayLine::process(float input)
{
    if (buffer.empty()) return input;
    
    float output = buffer[writeIndex];
    buffer[writeIndex] = input + output * feedback;
    writeIndex = (writeIndex + 1) % buffer.size();
    return output;
}

void SynthEngine::StateVariableFilter::setParams(float frequency, float resonance, float sampleRate)
{
    f = 2.0f * std::sin(juce::MathConstants<float>::pi * 
        std::min(frequency, sampleRate * 0.49f) / sampleRate);
    q = 1.0f / std::max(resonance, 0.5f);
}

float SynthEngine::StateVariableFilter::processLowpass(float input)
{
    low += f * band;
    high = input - low - q * band;
    band += f * high;
    return low;
}

float SynthEngine::StateVariableFilter::processBandpass(float input)
{
    low += f * band;
    high = input - low - q * band;
    band += f * high;
    return band;
}

float SynthEngine::StateVariableFilter::processHighpass(float input)
{
    low += f * band;
    high = input - low - q * band;
    band += f * high;
    return high;
}

float SynthEngine::FMOperator::generate(float modulation)
{
    float output = std::sin(2.0f * juce::MathConstants<float>::pi * (phase + modulation)) * amplitude;
    return output;
}

float SynthEngine::softClip(float input)
{
    // Soft clipping for warmth
    if (std::abs(input) < 0.7f)
        return input;
    
    return std::tanh(input * 1.5f) * 0.7f;
}

float SynthEngine::randomFloat()
{
    return randomDist(rng);
}

void SynthEngine::updateMorphPosition(float frequency)
{
    // Update morph position based on frequency
    morphX = std::sin(frequency * 0.01f) * 0.5f + 0.5f;
    morphY = std::cos(frequency * 0.007f) * 0.5f + 0.5f;
    
    // Update wavetable morph
    wavetable.morphPosition = (frequency / 100.0f);
    wavetable.morphPosition = std::fmod(wavetable.morphPosition, 
        static_cast<float>(WavetableOscillator::NUM_TABLES));
}

void SynthEngine::triggerGrain()
{
    // Find inactive grain
    for (auto& grain : grains)
    {
        if (!grain.active)
        {
            grain.active = true;
            grain.position = randomFloat() * 0.5f + 0.5f;
            grain.duration = 0.05f + randomFloat() * 0.1f;
            grain.pitch = 0.5f + randomFloat() * 2.0f;
            grain.amplitude = 0.2f + randomFloat() * 0.3f;
            grain.envelope = 0.0f;
            break;
        }
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