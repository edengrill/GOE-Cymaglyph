#include "SynthEngine.h"
#include <algorithm>

// Keep the exact same color schemes as before
const std::array<SynthEngine::ModeInfo, SynthEngine::NumModes> SynthEngine::modeInfoTable = {{
    {"Crystalline", "Glass harmonics", 
        juce::Colour(255, 105, 180), juce::Colour(135, 206, 250), juce::Colour(255, 182, 193)}, // Pink to Blue
    
    {"Silk Pad", "Lush analog pad",
        juce::Colour(255, 140, 0), juce::Colour(255, 215, 0), juce::Colour(255, 69, 0)}, // Orange to Yellow
    
    {"Nebula Drift", "Spectral clouds",
        juce::Colour(0, 255, 255), juce::Colour(240, 248, 255), juce::Colour(175, 238, 238)}, // Cyan to White
    
    {"Liquid Bass", "Deep sub bass",
        juce::Colour(128, 0, 128), juce::Colour(255, 215, 0), juce::Colour(238, 130, 238)}, // Purple to Gold
    
    {"Plasma Core", "Metallic morph",
        juce::Colour(0, 255, 0), juce::Colour(0, 128, 128), juce::Colour(0, 255, 127)}, // Green to Teal
    
    {"Cloud Nine", "Ethereal texture",
        juce::Colour(255, 0, 0), juce::Colour(255, 140, 0), juce::Colour(255, 69, 0)}, // Red to Amber
    
    {"Quantum Flux", "Glitch lead",
        juce::Colour(255, 0, 0), juce::Colour(0, 255, 0), juce::Colour(0, 0, 255)}, // Rainbow (RGB)
    
    {"Crystal Matrix", "Resonant glass",
        juce::Colour(192, 192, 192), juce::Colour(0, 191, 255), juce::Colour(224, 224, 224)}, // Silver to Blue
    
    {"Solar Wind", "Space breath",
        juce::Colour(139, 69, 19), juce::Colour(34, 139, 34), juce::Colour(107, 142, 35)}, // Brown to Green
    
    {"Void Resonance", "Deep space",
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
        case NebulaDrift:    output = generateNebulaDrift(phase, frequency); break;
        case LiquidBass:     output = generateLiquidBass(phase, frequency); break;
        case PlasmaCore:     output = generatePlasmaCore(phase, frequency); break;
        case CloudNine:      output = generateCloudNine(phase, frequency); break;
        case QuantumFlux:    output = generateQuantumFlux(phase, frequency); break;
        case CrystalMatrix:  output = generateCrystalMatrix(phase, frequency); break;
        case SolarWind:      output = generateSolarWind(phase, frequency); break;
        case VoidResonance:  output = generateVoidResonance(phase, frequency); break;
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

float SynthEngine::generateNebulaDrift(float phase, float frequency)
{
    // Evolving spectral clouds with harmonic dispersion
    
    // Layer 1: Spectral-morphed wavetable with phase dispersion
    float wavetableOut = wavetable.generate(phase);
    
    // Apply spectral warping
    float warpAmount = lfos[0].process() * 0.5f + 0.5f;
    wavetableOut = spectralWarp(wavetableOut, warpAmount * 2.0f);
    
    // Layer 2: Sub-harmonic drone
    float subPhase = phase * 0.5f;
    float subOsc = std::sin(subPhase * 2.0f * M_PI);
    subOsc = analogSaturate(subOsc * 2.0f) * 0.3f;
    
    // Layer 3: High frequency shimmer particles
    float shimmer = 0.0f;
    for (int i = 5; i < 12; i++)
    {
        float harmPhase = phase * float(i);
        float harmAmp = 1.0f / float(i * i);
        shimmer += std::sin(harmPhase * 2.0f * M_PI) * harmAmp;
    }
    shimmer *= lfos[1].process() * 0.15f;
    
    // Apply Shepard tone morphing
    float shepardPos = lfos[2].process() * 0.5f + 0.5f;
    float harmonicArray[32];
    for (int i = 0; i < 32; i++)
    {
        harmonicArray[i] = harmonics[i].amplitude;
    }
    applyShepardTone(harmonicArray, 32, shepardPos);
    
    // Mix spectral components
    float spectralMix = 0.0f;
    for (int i = 0; i < 8; i++)
    {
        float harmPhase = phase * (i + 1);
        spectralMix += std::sin(harmPhase * 2.0f * M_PI) * harmonicArray[i];
    }
    
    float output = wavetableOut * 0.4f + subOsc + shimmer + spectralMix * 0.2f;
    
    // Formant shifting filter
    float formantFreq = frequency * (2.0f + lfos[3].process());
    filters[0].setStateVariable(formantFreq, 3.0f, 44100.0f);
    output = filters[0].processBandpass(output);
    
    // Multi-tap granular delay
    delay.time = 0.1f + lfos[0].process() * 0.05f;
    delay.feedback = 0.7f;
    delay.mix = 0.4f;
    output = delay.process(output);
    
    // Infinite reverb
    reverb.roomSize = 0.95f;
    reverb.damping = 0.3f;
    reverb.wetLevel = 0.5f;
    output = output * 0.5f + reverb.process(output) * 0.5f;
    
    return softClip(output * 0.6f);
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

float SynthEngine::generatePlasmaCore(float phase, float frequency)
{
    // Aggressive morphing metallic textures with harmonic distortion
    
    // Layer 1: Dual wavetables with FM cross-modulation
    float wt1 = wavetable.generate(phase);
    float wt2Phase = phase * 1.01f; // Slight detune
    float wt2 = wavetable.generate(wt2Phase);
    
    // FM cross-modulation
    float modDepth = 0.5f + lfos[0].process() * 0.3f;
    float fm1 = std::sin((phase + wt2 * modDepth) * 2.0f * M_PI);
    float fm2 = std::sin((wt2Phase + wt1 * modDepth) * 2.0f * M_PI);
    
    // Layer 2: Harmonic saturator with odd/even emphasis
    float oddHarmonics = 0.0f;
    float evenHarmonics = 0.0f;
    
    for (int i = 1; i <= 9; i += 2) // Odd harmonics
    {
        float harmPhase = phase * float(i);
        oddHarmonics += std::sin(harmPhase * 2.0f * M_PI) / float(i);
    }
    
    for (int i = 2; i <= 8; i += 2) // Even harmonics
    {
        float harmPhase = phase * float(i);
        evenHarmonics += std::sin(harmPhase * 2.0f * M_PI) / float(i * 2);
    }
    
    // Morph between odd and even harmonics
    float morphPos = lfos[1].process() * 0.5f + 0.5f;
    float harmonicMix = oddHarmonics * (1.0f - morphPos) + evenHarmonics * morphPos;
    
    // Layer 3: Resonant feedback network
    static float feedbackBuffer = 0.0f;
    float resonantSignal = fm1 + feedbackBuffer * 0.3f;
    filters[0].setMoogLadder(frequency * 2.5f, 3.5f, 44100.0f);
    resonantSignal = filters[0].processMoogLadder(resonantSignal);
    feedbackBuffer = resonantSignal * 0.7f;
    
    // Mix layers
    float output = fm1 * 0.3f + fm2 * 0.3f + harmonicMix * 0.2f + resonantSignal * 0.2f;
    
    // Spectral warping
    float warpAmount = 1.0f + lfos[2].process();
    output = spectralWarp(output, warpAmount);
    
    // Tube saturation
    output = analogSaturate(output * 2.0f) * 0.7f;
    
    // Comb filtering for metallic resonance
    static float combBuffer[256] = {0};
    static int combIndex = 0;
    int combDelay = int(frequency / 100.0f);
    combDelay = std::max(1, std::min(255, combDelay));
    
    float combOut = combBuffer[(combIndex - combDelay + 256) % 256];
    combBuffer[combIndex] = output + combOut * 0.4f;
    combIndex = (combIndex + 1) % 256;
    output = output + combOut * 0.3f;
    
    // Aggressive compression
    output = hardClip(output * 1.5f) * 0.6f;
    
    return output * velocity;
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

float SynthEngine::generateQuantumFlux(float phase, float frequency)
{
    // Glitchy evolving lead with spectral discontinuities
    
    // Layer 1: Wavetable with random harmonic amplitude modulation
    float wt = wavetable.generate(phase);
    
    // Randomize harmonics every few samples for glitch effect
    if (sampleCounter % 128 == 0)
    {
        for (int i = 0; i < 8; i++)
        {
            harmonics[i].amplitude = randomFloat() * 0.5f + 0.5f;
        }
    }
    
    float harmonicGlitch = 0.0f;
    for (int i = 1; i < 8; i++)
    {
        float harmPhase = phase * float(i);
        harmonicGlitch += std::sin(harmPhase * 2.0f * M_PI) * harmonics[i].amplitude / float(i);
    }
    
    // Layer 2: Sync oscillator with formant control
    static float syncPhase = 0.0f;
    if (phase < 0.01f) syncPhase = 0.0f; // Hard sync
    syncPhase += (frequency * 1.5f) / 44100.0f;
    if (syncPhase >= 1.0f) syncPhase -= 1.0f;
    
    float syncOsc = std::sin(syncPhase * 2.0f * M_PI);
    
    // Formant filter on sync
    float formantFreq = 700.0f + lfos[0].process() * 1000.0f;
    filters[0].setStateVariable(formantFreq, 5.0f, 44100.0f);
    syncOsc = filters[0].processBandpass(syncOsc);
    
    // Layer 3: Noise-modulated resonator
    float noiseMod = randomFloat() * 0.1f;
    filters[1].setStateVariable(frequency * 4.0f, 8.0f, 44100.0f);
    float resonator = filters[1].processBandpass(noiseMod + wt * 0.3f);
    
    // Mix layers
    float output = wt * 0.3f + harmonicGlitch * 0.3f + syncOsc * 0.2f + resonator * 0.2f;
    
    // Bit crushing with sample rate reduction
    bitCrusher.bitDepth = 8.0f + lfos[1].process() * 4.0f;
    bitCrusher.sampleRateReduction = 4.0f + lfos[2].process() * 8.0f;
    output = bitCrusher.process(output);
    
    // Spectral freeze effect (randomly hold spectral snapshot)
    static float frozenSample = 0.0f;
    if (randomFloat() > 0.98f) // 2% chance to freeze
    {
        frozenSample = output;
    }
    output = output * 0.7f + frozenSample * 0.3f;
    
    // Phaser for movement
    output = phaser.process(output);
    
    // Aggressive limiting
    output = hardClip(output * 2.0f) * 0.5f;
    
    return output * velocity;
}

float SynthEngine::generateCrystalMatrix(float phase, float frequency)
{
    // Glassy resonant plucks with harmonic cascades
    
    // Layer 1: Karplus-Strong with harmonic excitation
    float excitation = 0.0f;
    if (phase < 0.01f)
    {
        // Harmonic excitation burst
        for (int h = 1; h <= 5; h++)
        {
            excitation += std::sin(randomFloat() * M_PI) / float(h);
        }
        excitation *= velocity;
        
        for (auto& string : strings)
        {
            string.setFrequency(frequency * float((&string - &strings[0]) + 1), 44100.0f);
        }
    }
    
    // Process through multiple resonant strings at harmonic intervals
    float stringOut = 0.0f;
    for (int i = 0; i < 4; i++)
    {
        float harmFreq = frequency * float(i + 1);
        strings[i].setFrequency(harmFreq, 44100.0f);
        strings[i].damping = 0.1f; // Very resonant
        float harmString = strings[i].process(excitation / float(i + 1));
        stringOut += harmString * std::exp(-float(i) * 0.3f); // Exponential decay
    }
    
    // Layer 2: Additive harmonics with exponential decay
    float additiveOut = 0.0f;
    for (int h = 1; h <= 12; h++)
    {
        float harmPhase = phase * float(h);
        float harmEnv = std::exp(-phase * 5.0f * h); // Faster decay for higher harmonics
        additiveOut += std::sin(harmPhase * 2.0f * M_PI) * harmEnv / float(h * h);
    }
    
    // Layer 3: Resonant filter bank (glass resonances)
    float filterBank = 0.0f;
    for (int i = 0; i < 3; i++)
    {
        float resonFreq = frequency * (2.0f + i * 1.5f);
        filters[i].setStateVariable(resonFreq, 12.0f, 44100.0f); // Very high Q
        filterBank += filters[i].processBandpass(stringOut + additiveOut) * 0.3f;
    }
    
    float output = stringOut * 0.4f + additiveOut * 0.3f + filterBank * 0.3f;
    
    // Pitch-shifted delays for harmonic cascades
    static float pitchBuffer[2048] = {0};
    static int pitchIndex = 0;
    
    // Write to buffer
    pitchBuffer[pitchIndex] = output;
    
    // Read with pitch shifts
    float cascade = 0.0f;
    for (int i = 1; i <= 3; i++)
    {
        int delayTime = int(44100.0f / (frequency * float(i * 2)));
        delayTime = std::min(2047, std::max(1, delayTime));
        int readIndex = (pitchIndex - delayTime + 2048) % 2048;
        cascade += pitchBuffer[readIndex] * 0.2f / float(i);
    }
    
    pitchIndex = (pitchIndex + 1) % 2048;
    output += cascade;
    
    // Shimmer reverb
    reverb.roomSize = 0.7f;
    reverb.damping = 0.2f; // Bright reverb
    reverb.wetLevel = 0.4f;
    float shimmer = reverb.process(output);
    
    // Harmonic enhancer
    float enhanced = output + analogSaturate(output * 3.0f) * 0.1f;
    
    return (enhanced * 0.6f + shimmer * 0.4f) * 0.7f;
}

float SynthEngine::generateSolarWind(float phase, float frequency)
{
    // Expansive breathing pads with spectral movement
    
    // Layer 1: Granular cloud with spectral freezing
    if (sampleCounter % 64 == 0) // Trigger new grain
    {
        triggerGrain();
    }
    
    float granularOut = 0.0f;
    for (auto& grain : grains)
    {
        if (grain.active)
        {
            grain.envelope += 1.0f / (grain.duration * 44100.0f);
            if (grain.envelope >= 1.0f)
            {
                grain.active = false;
                continue;
            }
            
            // Gaussian window
            float window = std::exp(-5.0f * std::pow(grain.envelope - 0.5f, 2));
            float grainPhase = phase * grain.pitch + grain.position;
            float grainSample = std::sin(grainPhase * 2.0f * M_PI) * window * grain.amplitude;
            
            // Spectral freezing
            static float frozenSpectrum[32] = {0};
            if (randomFloat() > 0.99f) // Occasionally freeze spectrum
            {
                frozenSpectrum[int(grain.position * 31)] = grainSample;
            }
            grainSample = grainSample * 0.7f + frozenSpectrum[int(grain.position * 31)] * 0.3f;
            
            granularOut += grainSample * (1.0f - std::abs(grain.pan));
        }
    }
    
    // Layer 2: Filtered noise with envelope following
    float noiseEnv = std::abs(std::sin(phase * 0.1f * M_PI));
    float filteredNoise = randomFloat() * noiseEnv * 0.1f;
    
    // Breathing filter
    float breathFreq = 200.0f + std::sin(phase * 0.05f * M_PI) * 800.0f + frequency;
    filters[0].setStateVariable(breathFreq, 2.0f, 44100.0f);
    filteredNoise = filters[0].processBandpass(filteredNoise);
    
    // Layer 3: Harmonic whisper oscillator
    float whisper = 0.0f;
    for (int h = 7; h <= 15; h++) // High harmonics only
    {
        float harmPhase = phase * float(h);
        float harmAmp = 1.0f / float(h * h);
        whisper += std::sin(harmPhase * 2.0f * M_PI) * harmAmp;
    }
    whisper *= lfos[0].process() * 0.1f;
    
    // Mix layers
    float output = granularOut * 0.4f + filteredNoise * 0.3f + whisper * 0.3f;
    
    // Infinite reverb
    reverb.roomSize = 0.99f;
    reverb.damping = 0.1f; // Very bright
    reverb.wetLevel = 0.8f;
    float infinite = reverb.process(output);
    
    // Spectral blur (all-pass diffusion network)
    static float diffusionBuffer[4][512] = {{0}};
    static int diffusionIndex[4] = {0};
    float blurred = output;
    
    for (int i = 0; i < 4; i++)
    {
        int delayTime = 37 + i * 89; // Prime numbers for inharmonic diffusion
        diffusionBuffer[i][diffusionIndex[i]] = blurred;
        int readIndex = (diffusionIndex[i] - delayTime + 512) % 512;
        float delayed = diffusionBuffer[i][readIndex];
        blurred = delayed * 0.7f + blurred * 0.3f;
        diffusionIndex[i] = (diffusionIndex[i] + 1) % 512;
    }
    
    // Auto-pan with Doppler effect
    float panLfo = std::sin(phase * 0.2f * M_PI);
    float dopplerShift = 1.0f + panLfo * 0.02f; // Slight pitch shift
    
    // Dimension expander
    auto [left, right] = dimension.process(infinite);
    
    // Mix everything
    output = blurred * 0.3f + infinite * 0.5f + (left + right) * 0.1f;
    
    return softClip(output * 0.5f);
}

float SynthEngine::generateVoidResonance(float phase, float frequency)
{
    // Deep evolving bass with upper harmonic tendrils
    
    // Layer 1: Sub-bass with harmonic tracking
    float subFreq = frequency * 0.5f; // One octave down
    float subPhase = phase * 0.5f;
    float subBass = std::sin(subPhase * 2.0f * M_PI);
    
    // Add sub-sub for extreme depth
    float subSubPhase = phase * 0.25f;
    float subSub = std::sin(subSubPhase * 2.0f * M_PI) * 0.5f;
    
    // Harmonic tracking - emphasize harmonics that align with the key
    float trackedHarmonics = 0.0f;
    for (int h = 2; h <= 5; h++)
    {
        float harmPhase = phase * float(h);
        float harmAmp = 1.0f / float(h);
        // Boost harmonics that are octaves or fifths
        if (h == 2 || h == 4) harmAmp *= 1.5f; // Octaves
        if (h == 3) harmAmp *= 1.2f; // Fifth
        trackedHarmonics += std::sin(harmPhase * 2.0f * M_PI) * harmAmp;
    }
    
    // Layer 2: Formant-morphed wavetable
    float wtOut = wavetable.generate(phase);
    
    // Formant morphing
    float formant1 = 700.0f + lfos[0].process() * 500.0f;
    float formant2 = 1220.0f + lfos[1].process() * 800.0f;
    
    filters[0].setStateVariable(formant1, 4.0f, 44100.0f);
    filters[1].setStateVariable(formant2, 4.0f, 44100.0f);
    
    float vowel1 = filters[0].processBandpass(wtOut);
    float vowel2 = filters[1].processBandpass(wtOut);
    float formantMorph = vowel1 * 0.5f + vowel2 * 0.5f;
    
    // Layer 3: Resonant pulse wave
    float pulseWidth = 0.3f + lfos[2].process() * 0.2f;
    float pulsePhase = std::fmod(phase, 1.0f);
    float pulseWave = pulsePhase < pulseWidth ? 1.0f : -1.0f;
    
    // Resonant filter on pulse
    filters[2].setMoogLadder(frequency * 4.0f, 4.0f, 44100.0f);
    float resonantPulse = filters[2].processMoogLadder(pulseWave);
    
    // Mix layers with emphasis on bass
    float output = (subBass * 0.4f + subSub * 0.2f) + 
                   trackedHarmonics * 0.15f + 
                   formantMorph * 0.15f + 
                   resonantPulse * 0.1f;
    
    // Multiband compression
    // Low band (sub)
    filters[3].setStateVariable(120.0f, 0.7f, 44100.0f);
    float lowBand = filters[3].processLowpass(output);
    lowBand = softClip(lowBand * 2.0f) * 0.5f;
    
    // Mid band
    float midBand = output - lowBand;
    midBand = softClip(midBand * 1.5f) * 0.6f;
    
    output = lowBand + midBand;
    
    // Spectral tilt (emphasize lows)
    float tilt = output - filters[3].processHighpass(output) * 0.3f;
    
    // Dimension expander for width
    dimension.size = 0.3f;
    dimension.diffusion = 0.5f;
    auto [left, right] = dimension.process(tilt);
    
    // Deep space reverb (subtle)
    reverb.roomSize = 0.7f;
    reverb.damping = 0.8f; // Dark reverb
    reverb.wetLevel = 0.15f;
    float spaceReverb = reverb.process((left + right) * 0.5f);
    
    output = tilt * 0.8f + spaceReverb * 0.2f;
    
    // Final limiting
    return softClip(output * 0.7f) * velocity;
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

float SynthEngine::spectralWarp(float value, float warp)
{
    if (std::abs(warp) < 0.01f) return value;
    
    float absValue = std::abs(value);
    float numerator = std::exp(warp * absValue) - 1.0f;
    float denominator = std::exp(warp) - 1.0f;
    
    return (value >= 0.0f) ? numerator / denominator : -numerator / denominator;
}

void SynthEngine::applyShepardTone(float* harmonics, int numHarmonics, float position)
{
    // Create Shepard tone effect by fading harmonics cyclically
    for (int i = 0; i < numHarmonics; i++)
    {
        float harmPos = std::fmod(position + float(i) / numHarmonics, 1.0f);
        float envelope = std::sin(harmPos * M_PI);
        harmonics[i] *= envelope;
    }
}

// Advanced processing implementations
float SynthEngine::BitCrusher::process(float input)
{
    // Bit depth reduction
    float levels = std::pow(2.0f, bitDepth);
    float quantized = std::round(input * levels) / levels;
    
    // Sample rate reduction
    sampleCounter++;
    int skipSamples = int(sampleRateReduction);
    if (sampleCounter >= skipSamples)
    {
        lastSample = quantized;
        sampleCounter = 0;
    }
    
    return lastSample;
}

float SynthEngine::Phaser::process(float input)
{
    // Update LFO
    lfoPhase += rate * 0.0001f;
    if (lfoPhase > 1.0f) lfoPhase -= 1.0f;
    
    float lfoValue = std::sin(lfoPhase * 2.0f * M_PI);
    float frequency = 200.0f + lfoValue * depth * 2000.0f;
    
    // Process through allpass stages
    float output = input;
    for (int i = 0; i < NUM_STAGES; i++)
    {
        float allpassCoeff = (1.0f - frequency / 22050.0f) / (1.0f + frequency / 22050.0f);
        float temp = output;
        output = -output + allpassCoeff * (output - allpassStates[i]);
        allpassStates[i] = temp;
    }
    
    return input + output * feedback;
}

std::pair<float, float> SynthEngine::DimensionExpander::process(float input)
{
    // Haas effect with diffusion
    int delayTime1 = int(size * 441.0f); // 10ms max
    int delayTime2 = int(size * 882.0f); // 20ms max
    
    delayBuffer[writeIndex] = input;
    
    int readIndex1 = (writeIndex - delayTime1 + 4096) % 4096;
    int readIndex2 = (writeIndex - delayTime2 + 4096) % 4096;
    
    float delayed1 = delayBuffer[readIndex1];
    float delayed2 = delayBuffer[readIndex2];
    
    // Apply diffusion
    float diffused1 = delayed1 * (1.0f - diffusion) + delayed2 * diffusion * 0.5f;
    float diffused2 = delayed2 * (1.0f - diffusion) + delayed1 * diffusion * 0.5f;
    
    writeIndex = (writeIndex + 1) % 4096;
    
    return {input + diffused1 * 0.3f, input + diffused2 * 0.3f};
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