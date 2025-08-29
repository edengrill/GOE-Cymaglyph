#include "SynthEngine.h"
#include <algorithm>

// Keep the exact same color schemes as before
const std::array<SynthEngine::ModeInfo, SynthEngine::NumModes> SynthEngine::modeInfoTable = {{
    {"Crystalline", "Glass harmonics", 
        juce::Colour(255, 105, 180), juce::Colour(135, 206, 250), juce::Colour(255, 182, 193)}, // Pink to Blue
    
    {"Silk Pad", "Lush analog pad",
        juce::Colour(255, 140, 0), juce::Colour(255, 215, 0), juce::Colour(255, 69, 0)}, // Orange to Yellow
    
    {"Velvet Keys", "Electric piano",
        juce::Colour(0, 255, 255), juce::Colour(240, 248, 255), juce::Colour(175, 238, 238)}, // Cyan to White
    
    {"Liquid Bass", "Deep sub bass",
        juce::Colour(128, 0, 128), juce::Colour(255, 215, 0), juce::Colour(238, 130, 238)}, // Purple to Gold
    
    {"Vintage Brass", "Analog brass",
        juce::Colour(0, 255, 0), juce::Colour(0, 128, 128), juce::Colour(0, 255, 127)}, // Green to Teal
    
    {"Cloud Nine", "Ethereal texture",
        juce::Colour(255, 0, 0), juce::Colour(255, 140, 0), juce::Colour(255, 69, 0)}, // Red to Amber
    
    {"Golden Lead", "Cutting lead",
        juce::Colour(255, 0, 0), juce::Colour(0, 255, 0), juce::Colour(0, 0, 255)}, // Rainbow (RGB)
    
    {"Dream Pluck", "Lush pluck",
        juce::Colour(192, 192, 192), juce::Colour(0, 191, 255), juce::Colour(224, 224, 224)}, // Silver to Blue
    
    {"Ambient Wash", "Ocean texture",
        juce::Colour(139, 69, 19), juce::Colour(34, 139, 34), juce::Colour(107, 142, 35)}, // Brown to Green
    
    {"Prophet Poly", "Vintage poly",
        juce::Colour(255, 0, 255), juce::Colour(138, 43, 226), juce::Colour(255, 105, 180)} // Magenta to Violet
}};

SynthEngine::SynthEngine() : rng(std::random_device{}()), randomDist(-1.0f, 1.0f)
{
    // Initialize professional wavetables
    wavetable.initialize();
    
    // Initialize reverb
    reverb.initialize();
    
    // Initialize delay line
    delay.resize(static_cast<int>(44100 * 0.5f)); // 500ms max delay
    
    // Initialize grain buffer with rich harmonic content
    grainBuffer.resize(8192);
    for (size_t i = 0; i < grainBuffer.size(); i++)
    {
        float t = static_cast<float>(i) / grainBuffer.size();
        // Create lush texture
        grainBuffer[i] = std::sin(2.0f * juce::MathConstants<float>::pi * t) * 0.5f;
        grainBuffer[i] += std::sin(4.0f * juce::MathConstants<float>::pi * t) * 0.25f;
        grainBuffer[i] += std::sin(6.0f * juce::MathConstants<float>::pi * t) * 0.125f;
        // Apply envelope
        grainBuffer[i] *= std::exp(-t * 3.0f) * (1.0f - t);
    }
    
    // Initialize layers with slight detuning for richness
    for (int i = 0; i < 4; i++)
    {
        layers[i].detune = (i - 1.5f) * 0.002f; // Slight detuning
        layers[i].pan = (i - 1.5f) * 0.25f; // Stereo spread
    }
    
    // Initialize FM operators for electric piano
    float ratios[] = {1.0f, 14.0f, 1.0f, 1.0f, 0.5f, 1.0f};
    for (size_t i = 0; i < fmOperators.size(); i++)
    {
        fmOperators[i].frequency = ratios[i];
        fmOperators[i].amplitude = 1.0f;
    }
    
    // Initialize strings for Dream Pluck
    for (auto& string : strings)
    {
        string.setFrequency(440.0f, 44100.0f);
    }
}

void SynthEngine::reset()
{
    // Reset all oscillator phases
    currentPhase = 0.0f;
    for (auto& layer : layers)
        layer.phase = 0.0f;
    
    for (auto& op : fmOperators)
    {
        op.phase = 0.0f;
        op.lastOutput = 0.0f;
    }
    
    for (auto& grain : grains)
        grain.active = false;
    
    // Reset envelopes
    for (auto& env : envelopes)
    {
        env.level = 0.0f;
        env.state = 0.0f;
    }
}

float SynthEngine::generateSample(float phase, float frequency, int modeIndex)
{
    // Track frequency changes
    if (std::abs(frequency - lastFrequency) > 0.1f)
    {
        lastFrequency = frequency;
    }
    
    float output = 0.0f;
    
    switch (modeIndex)
    {
        case Crystalline:    output = generateCrystalline(phase, frequency); break;
        case SilkPad:        output = generateSilkPad(phase, frequency); break;
        case VelvetKeys:     output = generateVelvetKeys(phase, frequency); break;
        case LiquidBass:     output = generateLiquidBass(phase, frequency); break;
        case VintageBrass:   output = generateVintageBrass(phase, frequency); break;
        case CloudNine:      output = generateCloudNine(phase, frequency); break;
        case GoldenLead:     output = generateGoldenLead(phase, frequency); break;
        case DreamPluck:     output = generateDreamPluck(phase, frequency); break;
        case AmbientWash:    output = generateAmbientWash(phase, frequency); break;
        case ProphetPoly:    output = generateProphetPoly(phase, frequency); break;
        default:             output = 0.0f; break;
    }
    
    // Professional limiting
    return softClip(output);
}

float SynthEngine::generateCrystalline(float phase, float frequency)
{
    // Keep the original Crystalline sound
    float output = wavetable.generate(phase);
    
    // Add inharmonic partials for bell-like quality
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 2.76f) * 0.15f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 5.4f) * 0.1f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 8.93f) * 0.05f;
    
    // Filter for smoothness
    filters[0].setStateVariable(frequency * 4.0f, 2.0f, 44100.0f);
    output = filters[0].processLowpass(output);
    
    // Add shimmer with chorus
    output = chorus.process(output);
    
    // Subtle reverb
    float reverbSignal = reverb.process(output * 0.3f);
    
    return output * 0.6f + reverbSignal * 0.4f;
}

float SynthEngine::generateSilkPad(float phase, float frequency)
{
    // 3-layer detuned saw waves for lush pad
    float output = 0.0f;
    
    // Update layer phases
    for (int i = 0; i < 3; i++)
    {
        layers[i].phase += (frequency * (1.0f + layers[i].detune)) / 44100.0f;
        if (layers[i].phase >= 1.0f) layers[i].phase -= 1.0f;
        
        // Generate saw wave
        float saw = 2.0f * layers[i].phase - 1.0f;
        
        // Apply formant filtering for warmth
        filters[i].setStateVariable(800.0f + i * 200.0f, 3.0f, 44100.0f);
        saw = filters[i].processBandpass(saw);
        
        output += saw * (1.0f / (i + 1));
    }
    
    // Warm filter sweep
    float cutoff = 2000.0f + std::sin(phase * 0.1f) * 1000.0f;
    filters[3].setMoogLadder(cutoff, 0.3f, 44100.0f);
    output = filters[3].processMoogLadder(output);
    
    // Built-in ensemble chorus for width
    chorus.rate = 0.3f;
    chorus.depth = 0.4f;
    chorus.mix = 0.5f;
    output = chorus.process(output);
    
    // Lush reverb
    reverb.roomSize = 0.8f;
    reverb.wetLevel = 0.4f;
    float reverbSignal = reverb.process(output);
    
    // Analog warmth
    output = analogSaturate(output * 0.5f + reverbSignal * 0.5f);
    
    return output * 0.4f * velocity;
}

float SynthEngine::generateVelvetKeys(float phase, float frequency)
{
    // DX7-style FM synthesis for electric piano
    
    // Update FM operator phases
    for (size_t i = 0; i < 6; i++)
    {
        fmOperators[i].phase += (frequency * fmOperators[i].frequency) / 44100.0f;
        if (fmOperators[i].phase >= 1.0f) fmOperators[i].phase -= 1.0f;
    }
    
    // Algorithm 5 - Classic EP
    float mod1 = fmOperators[1].generate(0.0f) * 14.0f * velocity;
    float carrier1 = fmOperators[0].generate(mod1);
    
    float mod2 = fmOperators[3].generate(0.0f) * 1.0f;
    float carrier2 = fmOperators[2].generate(mod2);
    
    float bell = fmOperators[4].generate(0.0f) * 0.3f; // Bell component
    
    float output = (carrier1 * 0.6f + carrier2 * 0.3f + bell) * velocity;
    
    // Tine resonance simulation
    filters[0].setStateVariable(frequency * 2.1f, 8.0f, 44100.0f);
    float resonance = filters[0].processBandpass(output) * 0.2f;
    
    output += resonance;
    
    // Vintage chorus for classic EP sound
    chorus.rate = 0.5f;
    chorus.depth = 0.2f;
    chorus.mix = 0.3f;
    output = chorus.process(output);
    
    // Cabinet simulation
    filters[1].setStateVariable(4000.0f, 0.7f, 44100.0f);
    output = filters[1].processLowpass(output);
    
    // Room reverb
    reverb.roomSize = 0.3f;
    reverb.wetLevel = 0.2f;
    output = output * 0.8f + reverb.process(output) * 0.2f;
    
    return analogSaturate(output * 0.5f);
}

float SynthEngine::generateLiquidBass(float phase, float frequency)
{
    // Dual oscillator with sub-harmonic synthesis
    
    // Main oscillator - sine for fundamental
    float fundamental = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    
    // Sub oscillator - one octave down
    layers[0].phase += (frequency * 0.5f) / 44100.0f;
    if (layers[0].phase >= 1.0f) layers[0].phase -= 1.0f;
    float sub = std::sin(2.0f * juce::MathConstants<float>::pi * layers[0].phase);
    
    // Second harmonic for presence
    float second = std::sin(4.0f * juce::MathConstants<float>::pi * phase) * 0.3f;
    
    // Mix oscillators
    float output = fundamental * 0.6f + sub * 0.5f + second * 0.2f;
    
    // Spectral warping for movement (Vital-inspired)
    float warp = std::pow(std::abs(output), 1.5f) * (output > 0 ? 1.0f : -1.0f);
    output = mixLayers(output, warp, 0.3f);
    
    // Filter with envelope following
    float envFollow = std::abs(output) * 2.0f + 0.5f;
    float cutoff = 200.0f + envFollow * 500.0f;
    filters[0].setMoogLadder(cutoff, 0.4f + velocity * 0.3f, 44100.0f);
    output = filters[0].processMoogLadder(output);
    
    // Compression for punch
    output = analogSaturate(output * 2.0f) * 0.5f;
    
    // Subtle chorus for width
    chorus.rate = 0.1f;
    chorus.depth = 0.1f;
    chorus.mix = 0.1f;
    output = chorus.process(output);
    
    return output * 0.6f;
}

float SynthEngine::generateVintageBrass(float phase, float frequency)
{
    // Additive synthesis with dynamic formants
    updateHarmonics(frequency, VintageBrass);
    
    float output = 0.0f;
    
    // Generate harmonics with brass-like envelope
    for (int h = 0; h < 16; h++)
    {
        harmonics[h].phase += (frequency * (h + 1)) / 44100.0f;
        if (harmonics[h].phase >= 1.0f) harmonics[h].phase -= 1.0f;
        
        // Brass harmonics emphasis
        float amp = 1.0f / (h + 1);
        if (h == 1) amp *= 1.5f;  // Emphasize 2nd harmonic
        if (h == 2) amp *= 1.3f;  // Emphasize 3rd harmonic
        if (h == 4) amp *= 1.2f;  // Emphasize 5th harmonic
        
        // Apply breath control simulation
        amp *= (0.7f + velocity * 0.3f);
        
        output += std::sin(2.0f * juce::MathConstants<float>::pi * harmonics[h].phase) * amp;
    }
    
    output *= 0.15f;
    
    // Formant filter for brass character
    filters[0].setStateVariable(1500.0f, 2.0f, 44100.0f);
    output = filters[0].processBandpass(output);
    
    // Dynamic brightness based on velocity
    float brightness = 3000.0f + velocity * 2000.0f;
    filters[1].setStateVariable(brightness, 0.7f, 44100.0f);
    output = filters[1].processLowpass(output);
    
    // Add growl with soft saturation
    output = analogSaturate(output * (1.0f + velocity));
    
    // Hall reverb
    reverb.roomSize = 0.6f;
    reverb.wetLevel = 0.25f;
    output = output * 0.75f + reverb.process(output) * 0.25f;
    
    return output * 0.5f;
}

float SynthEngine::generateCloudNine(float phase, float frequency)
{
    // Granular pad with spectral freezing
    
    // Trigger grains periodically
    grainCounter++;
    if (grainCounter > static_cast<int>(44100.0f / 30.0f)) // 30 grains per second
    {
        grainCounter = 0;
        triggerGrain();
    }
    
    float output = 0.0f;
    
    // Process active grains with spectral spreading
    for (auto& grain : grains)
    {
        if (grain.active)
        {
            // Gaussian envelope for smooth grains
            float env = std::exp(-std::pow((grain.envelope - 0.5f) * 4.0f, 2.0f));
            
            // Read from buffer with pitch variation
            int idx = static_cast<int>(grain.position * grainBuffer.size()) % grainBuffer.size();
            float sample = grainBuffer[idx];
            
            // Apply spectral freezing effect
            float frozen = std::sin(2.0f * juce::MathConstants<float>::pi * grain.position * grain.pitch);
            sample = mixLayers(sample, frozen, 0.5f);
            
            output += sample * env * grain.amplitude * 0.1f;
            
            // Update grain
            grain.envelope += 1.0f / (grain.duration * 44100.0f);
            grain.position += grain.pitch / 44100.0f;
            
            if (grain.envelope >= 1.0f)
                grain.active = false;
        }
    }
    
    // Add pad layer
    float pad = wavetable.generate(phase) * 0.2f;
    output += pad;
    
    // Ethereal filtering
    filters[0].setStateVariable(800.0f + std::sin(phase * 0.05f) * 400.0f, 3.0f, 44100.0f);
    output = filters[0].processLowpass(output);
    
    // Heavy chorus for width
    chorus.rate = 0.2f;
    chorus.depth = 0.5f;
    chorus.mix = 0.6f;
    output = chorus.process(output);
    
    // Massive reverb
    reverb.roomSize = 0.95f;
    reverb.damping = 0.7f;
    reverb.wetLevel = 0.6f;
    float reverbSignal = reverb.process(output);
    
    // Delay for space
    delay.time = 0.375f;
    delay.feedback = 0.4f;
    delay.mix = 0.3f;
    float delaySignal = delay.process(output);
    
    return (output * 0.3f + reverbSignal * 0.5f + delaySignal * 0.2f) * 0.4f;
}

float SynthEngine::generateGoldenLead(float phase, float frequency)
{
    // Wavetable lead with analog filter
    
    // Dual wavetable oscillators slightly detuned
    float osc1 = wavetable.generate(phase);
    
    layers[0].phase += (frequency * 1.003f) / 44100.0f;
    if (layers[0].phase >= 1.0f) layers[0].phase -= 1.0f;
    float osc2 = wavetable.generate(layers[0].phase) * 0.7f;
    
    float output = osc1 + osc2;
    
    // Add bite with sync
    if (phase < 0.01f)
        layers[1].phase = 0.0f;
    layers[1].phase += (frequency * 2.0f) / 44100.0f;
    if (layers[1].phase >= 1.0f) layers[1].phase -= 1.0f;
    output += std::sin(2.0f * juce::MathConstants<float>::pi * layers[1].phase) * 0.2f;
    
    // Moog ladder filter with envelope
    float cutoff = 1000.0f + velocity * 3000.0f;
    filters[0].setMoogLadder(cutoff, 0.3f + velocity * 0.4f, 44100.0f);
    output = filters[0].processMoogLadder(output);
    
    // Add presence
    filters[1].setStateVariable(3000.0f, 2.0f, 44100.0f);
    float presence = filters[1].processHighpass(output) * 0.2f;
    output += presence;
    
    // Vintage saturation
    output = analogSaturate(output * 1.5f) * 0.7f;
    
    // Short delay for thickness
    delay.time = 0.02f;
    delay.feedback = 0.2f;
    delay.mix = 0.15f;
    output = delay.process(output);
    
    // Small room reverb
    reverb.roomSize = 0.2f;
    reverb.wetLevel = 0.15f;
    output = output * 0.85f + reverb.process(output) * 0.15f;
    
    return output * 0.5f * velocity;
}

float SynthEngine::generateDreamPluck(float phase, float frequency)
{
    // Karplus-Strong with chorus ensemble
    
    // Excitation on note start
    float excitation = 0.0f;
    if (phase < 0.01f)
    {
        for (auto& string : strings)
        {
            string.setFrequency(frequency, 44100.0f);
        }
        excitation = (randomFloat() * 0.5f + 0.5f) * velocity;
    }
    
    float output = 0.0f;
    
    // Multiple strings for chorus effect
    for (int i = 0; i < 3; i++)
    {
        // Slight detuning for each string
        strings[i].setFrequency(frequency * (1.0f + i * 0.002f), 44100.0f);
        float stringOut = strings[i].process(excitation * (1.0f - i * 0.2f));
        output += stringOut * (1.0f / (i + 1));
    }
    
    // Add fundamental for body
    output += std::sin(2.0f * juce::MathConstants<float>::pi * phase) * 0.1f;
    
    // Resonant body filter
    filters[0].setStateVariable(frequency * 2.0f, 4.0f, 44100.0f);
    float resonance = filters[0].processBandpass(output) * 0.3f;
    output += resonance;
    
    // Juno-style chorus
    chorus.rate = 0.6f;
    chorus.depth = 0.4f;
    chorus.mix = 0.5f;
    output = chorus.process(output);
    
    // Gentle high-freq rolloff
    filters[1].setStateVariable(8000.0f, 0.7f, 44100.0f);
    output = filters[1].processLowpass(output);
    
    // Plate reverb
    reverb.roomSize = 0.5f;
    reverb.damping = 0.4f;
    reverb.wetLevel = 0.3f;
    output = output * 0.7f + reverb.process(output) * 0.3f;
    
    return output * 0.6f;
}

float SynthEngine::generateAmbientWash(float phase, float frequency)
{
    // Filtered noise with reverb synthesis
    
    // Multiple noise sources
    float noise = randomFloat() * 0.1f;
    float darkNoise = randomFloat() * 0.05f;
    
    // Ocean wave LFO
    float wave1 = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.13f);
    float wave2 = std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.17f);
    float waveMod = (wave1 + wave2) * 0.5f;
    
    // Resonant filtering for ocean character
    float cutoff = 500.0f + waveMod * 300.0f + frequency;
    filters[0].setStateVariable(cutoff, 3.0f, 44100.0f);
    float filtered = filters[0].processBandpass(noise);
    
    // Dark layer
    filters[1].setStateVariable(200.0f, 2.0f, 44100.0f);
    float dark = filters[1].processLowpass(darkNoise);
    
    // Tonal element for musicality
    float tonal = std::sin(2.0f * juce::MathConstants<float>::pi * phase) * 0.05f;
    tonal += std::sin(2.0f * juce::MathConstants<float>::pi * phase * 0.5f) * 0.03f;
    
    float output = filtered + dark + tonal;
    
    // Multi-tap delay network for space
    delay.time = 0.3f;
    delay.feedback = 0.5f;
    delay.mix = 0.4f;
    float delayed = delay.process(output);
    
    // Massive reverb
    reverb.roomSize = 0.98f;
    reverb.damping = 0.8f;
    reverb.wetLevel = 0.7f;
    float reverbSignal = reverb.process(output + delayed * 0.5f);
    
    // Slow chorus for movement
    chorus.rate = 0.1f;
    chorus.depth = 0.3f;
    chorus.mix = 0.4f;
    output = chorus.process(reverbSignal);
    
    // Gentle compression
    output = softClip(output * 2.0f) * 0.5f;
    
    return output * 0.4f;
}

float SynthEngine::generateProphetPoly(float phase, float frequency)
{
    // Virtual analog polysynth
    
    float output = 0.0f;
    
    // Two oscillators per voice
    // Osc 1: Saw
    float saw1 = 2.0f * phase - 1.0f;
    
    // Osc 2: Pulse with PWM
    layers[0].phase += frequency / 44100.0f;
    if (layers[0].phase >= 1.0f) layers[0].phase -= 1.0f;
    float pwm = 0.5f + std::sin(phase * 0.3f) * 0.3f;
    float pulse = layers[0].phase < pwm ? 1.0f : -1.0f;
    
    // Mix oscillators
    output = saw1 * 0.5f + pulse * 0.4f;
    
    // Sub oscillator
    layers[1].phase += (frequency * 0.5f) / 44100.0f;
    if (layers[1].phase >= 1.0f) layers[1].phase -= 1.0f;
    float sub = layers[1].phase < 0.5f ? 1.0f : -1.0f;
    output += sub * 0.2f;
    
    // Classic ladder filter
    float cutoff = 1500.0f + velocity * 2000.0f;
    filters[0].setMoogLadder(cutoff, 0.3f, 44100.0f);
    output = filters[0].processMoogLadder(output);
    
    // Vintage chorus
    chorus.rate = 0.4f;
    chorus.depth = 0.25f;
    chorus.mix = 0.3f;
    output = chorus.process(output);
    
    // Analog warmth
    output = analogSaturate(output * 1.2f);
    
    // Studio reverb
    reverb.roomSize = 0.4f;
    reverb.damping = 0.5f;
    reverb.wetLevel = 0.2f;
    output = output * 0.8f + reverb.process(output) * 0.2f;
    
    return output * 0.5f * velocity;
}

// Helper function implementations

void SynthEngine::WavetableOscillator::initialize()
{
    // Create professional wavetables with smooth morphing
    for (int table = 0; table < NUM_TABLES; table++)
    {
        for (int i = 0; i < TABLE_SIZE; i++)
        {
            float phase = static_cast<float>(i) / TABLE_SIZE;
            tables[table][i] = 0.0f;
            
            // Progressive harmonic complexity
            int numHarmonics = 1 + table;
            for (int h = 1; h <= numHarmonics; h++)
            {
                // Natural harmonic rolloff
                float amplitude = 1.0f / (h * (1.0f + table * 0.1f));
                tables[table][i] += std::sin(2.0f * juce::MathConstants<float>::pi * phase * h) * amplitude;
            }
            
            // Normalize with soft knee
            tables[table][i] = std::tanh(tables[table][i] * 0.5f);
        }
    }
}

float SynthEngine::WavetableOscillator::generate(float phase)
{
    int tableA = static_cast<int>(morphPosition);
    int tableB = (tableA + 1) % NUM_TABLES;
    float blend = morphPosition - tableA;
    
    float floatIndex = phase * TABLE_SIZE;
    int index = static_cast<int>(floatIndex) % TABLE_SIZE;
    int nextIndex = (index + 1) % TABLE_SIZE;
    float frac = floatIndex - index;
    
    float sampleA = tables[tableA][index] * (1.0f - frac) + tables[tableA][nextIndex] * frac;
    float sampleB = tables[tableB][index] * (1.0f - frac) + tables[tableB][nextIndex] * frac;
    
    return sampleA * (1.0f - blend) + sampleB * blend;
}

void SynthEngine::Filter::setStateVariable(float frequency, float resonance, float sampleRate)
{
    f = 2.0f * std::sin(juce::MathConstants<float>::pi * 
        std::min(frequency, sampleRate * 0.49f) / sampleRate);
    q = 1.0f / std::max(resonance, 0.5f);
}

float SynthEngine::Filter::processLowpass(float input)
{
    low += f * band;
    high = input - low - q * band;
    band += f * high;
    return low;
}

float SynthEngine::Filter::processBandpass(float input)
{
    low += f * band;
    high = input - low - q * band;
    band += f * high;
    return band;
}

float SynthEngine::Filter::processHighpass(float input)
{
    low += f * band;
    high = input - low - q * band;
    band += f * high;
    return high;
}

float SynthEngine::Filter::processNotch(float input)
{
    low += f * band;
    high = input - low - q * band;
    band += f * high;
    return low + high;
}

void SynthEngine::Filter::setMoogLadder(float frequency, float resonance, float sampleRate)
{
    float fc = frequency / sampleRate;
    float f = fc * 0.5f;
    float fc2 = fc * fc;
    float fc3 = fc2 * fc;
    
    float fcr = 1.8730f * fc3 + 0.4955f * fc2 - 0.6490f * fc + 0.9988f;
    float acr = -3.9364f * fc2 + 1.8409f * fc + 0.9968f;
    
    feedback = resonance * (1.0f - 0.15f * fc2);
    
    // Calculate coefficients
    float k = std::tan(juce::MathConstants<float>::pi * f);
    float k2 = k * k;
    float bh = 1.0f + k / resonance + k2;
}

float SynthEngine::Filter::processMoogLadder(float input)
{
    // 4-stage ladder filter
    float sigma = feedback * stage[3];
    
    // Saturate feedback
    sigma = std::tanh(sigma);
    
    input = std::tanh(input - sigma);
    
    // Process stages
    stage[0] = input - delay[0];
    delay[0] = input;
    
    stage[1] = stage[0] - delay[1];
    delay[1] = stage[0];
    
    stage[2] = stage[1] - delay[2];
    delay[2] = stage[1];
    
    stage[3] = stage[2] - delay[3];
    delay[3] = stage[2];
    
    return stage[3];
}

float SynthEngine::Envelope::process(bool gate)
{
    if (gate)
    {
        if (state < 1.0f)
        {
            state += 1.0f / (attack * 44100.0f);
            if (state >= 1.0f)
            {
                state = 1.0f;
                level = 1.0f;
            }
            else
            {
                level = state;
            }
        }
        else if (level > sustain)
        {
            level -= (1.0f - sustain) / (decay * 44100.0f);
            if (level < sustain)
                level = sustain;
        }
    }
    else
    {
        level -= 1.0f / (release * 44100.0f);
        if (level < 0.0f)
        {
            level = 0.0f;
            state = 0.0f;
        }
    }
    
    return level;
}

float SynthEngine::LFO::process()
{
    phase += frequency / 44100.0f;
    if (phase >= 1.0f) phase -= 1.0f;
    return std::sin(2.0f * juce::MathConstants<float>::pi * phase) * depth;
}

float SynthEngine::Chorus::process(float input)
{
    // Write to delay buffer
    buffer[writeIndex] = input;
    
    // LFO for modulation
    lfoPhase += rate / 44100.0f;
    if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    float lfo = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase);
    
    // Calculate delay time
    float delayTime = 0.02f + depth * 0.02f * lfo;
    int delaySamples = static_cast<int>(delayTime * 44100.0f);
    
    // Read from delay buffer
    int readIndex = writeIndex - delaySamples;
    if (readIndex < 0) readIndex += MAX_DELAY;
    
    float delayed = buffer[readIndex];
    
    // Update write index
    writeIndex = (writeIndex + 1) % MAX_DELAY;
    
    // Mix dry and wet
    return input * (1.0f - mix) + delayed * mix;
}

void SynthEngine::Reverb::initialize()
{
    // Initialize comb filters with prime number delays
    int combDelays[] = {1557, 1617, 1491, 1422, 1277, 1356, 1188, 1116};
    for (int i = 0; i < NUM_COMBS; i++)
    {
        combs[i].buffer.resize(combDelays[i], 0.0f);
        combs[i].feedback = 0.84f;
        combs[i].damp = 0.2f;
    }
    
    // Initialize allpass filters
    int allpassDelays[] = {556, 441, 341, 225};
    for (int i = 0; i < NUM_ALLPASS; i++)
    {
        allpasses[i].buffer.resize(allpassDelays[i], 0.0f);
        allpasses[i].feedback = 0.5f;
    }
}

float SynthEngine::Reverb::process(float input)
{
    float output = 0.0f;
    
    // Process comb filters in parallel
    for (auto& comb : combs)
    {
        float y = comb.buffer[comb.index];
        comb.lastOut = y * (1.0f - damping) + comb.lastOut * damping;
        comb.buffer[comb.index] = input + comb.lastOut * comb.feedback * roomSize;
        comb.index = (comb.index + 1) % comb.buffer.size();
        output += y;
    }
    
    output *= 0.125f; // Scale down
    
    // Process allpass filters in series
    for (auto& allpass : allpasses)
    {
        float bufOut = allpass.buffer[allpass.index];
        float inSum = output + bufOut * allpass.feedback;
        allpass.buffer[allpass.index] = inSum;
        allpass.index = (allpass.index + 1) % allpass.buffer.size();
        output = bufOut - inSum * allpass.feedback;
    }
    
    return output * wetLevel;
}

void SynthEngine::DelayLine::resize(int size)
{
    buffer.resize(size, 0.0f);
    writeIndex = 0;
}

float SynthEngine::DelayLine::process(float input)
{
    if (buffer.empty()) return input;
    
    // Write input
    buffer[writeIndex] = input;
    
    // Calculate read position
    int delaySamples = static_cast<int>(time * 44100.0f);
    int readIndex = writeIndex - delaySamples;
    if (readIndex < 0) readIndex += buffer.size();
    
    // Read delayed signal
    float delayed = buffer[readIndex];
    
    // Apply feedback
    buffer[writeIndex] += delayed * feedback;
    
    // Update write position
    writeIndex = (writeIndex + 1) % buffer.size();
    
    // Mix dry and wet
    return input * (1.0f - mix) + delayed * mix;
}

float SynthEngine::FMOperator::generate(float modulation)
{
    float out = std::sin(2.0f * juce::MathConstants<float>::pi * (phase + modulation + lastOutput * feedback));
    lastOutput = out;
    return out * amplitude;
}

void SynthEngine::KarplusStrong::setFrequency(float freq, float sampleRate)
{
    int size = static_cast<int>(sampleRate / freq);
    if (size != delayLine.size())
    {
        delayLine.resize(size, 0.0f);
        writeIndex = 0;
    }
}

float SynthEngine::KarplusStrong::process(float excitation)
{
    if (delayLine.empty()) return excitation;
    
    float output = delayLine[writeIndex];
    
    // Karplus-Strong algorithm with damping
    float filtered = (output + lastSample) * 0.5f * feedback * (1.0f - damping);
    lastSample = output;
    
    delayLine[writeIndex] = excitation + filtered;
    writeIndex = (writeIndex + 1) % delayLine.size();
    
    return output;
}

float SynthEngine::softClip(float input)
{
    if (std::abs(input) < 0.7f)
        return input;
    return std::tanh(input * 1.2f) * 0.8f;
}

float SynthEngine::hardClip(float input)
{
    return std::max(-1.0f, std::min(1.0f, input));
}

float SynthEngine::analogSaturate(float input)
{
    // Tube-style saturation with even harmonics
    float x = input * 0.7f;
    float x2 = x * x;
    float x3 = x2 * x;
    
    // Add even harmonics for warmth
    float output = x + x2 * 0.1f - x3 * 0.05f;
    
    return std::tanh(output * 1.5f) * 0.7f;
}

float SynthEngine::randomFloat()
{
    return randomDist(rng);
}

void SynthEngine::triggerGrain()
{
    for (auto& grain : grains)
    {
        if (!grain.active)
        {
            grain.active = true;
            grain.position = randomFloat() * 0.5f + 0.5f;
            grain.duration = 0.1f + randomFloat() * 0.2f;
            grain.pitch = 0.8f + randomFloat() * 0.4f;
            grain.amplitude = 0.3f + randomFloat() * 0.4f;
            grain.envelope = 0.0f;
            grain.pan = randomFloat() * 0.5f;
            break;
        }
    }
}

void SynthEngine::updateHarmonics(float frequency, int mode)
{
    // Update harmonic amplitudes based on mode
    for (int i = 0; i < 32; i++)
    {
        harmonics[i].frequency = frequency * (i + 1);
        harmonics[i].amplitude = 1.0f / (i + 1);
    }
}

float SynthEngine::mixLayers(float dry, float wet, float mix)
{
    return dry * (1.0f - mix) + wet * mix;
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