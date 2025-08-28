#include "EnhancedVisualizer.h"
#include <cmath>

EnhancedVisualizer::EnhancedVisualizer(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
    // Initialize mode tables
    squareModes = CymaglyphModes::getSquareModes();
    
    // Initialize grain field
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grainField[i][j].x = i / float(GRID_SIZE - 1);
            grainField[i][j].y = j / float(GRID_SIZE - 1);
            grainField[i][j].phase = juce::Random::getSystemRandom().nextFloat() * 2.0f * juce::MathConstants<float>::pi;
            grainField[i][j].amplitude = 0.0f;
            grainField[i][j].displacement = 0.0f;
            grainField[i][j].size = 1.0f;
            grainField[i][j].brightness = 1.0f;
        }
    }
    
    // Set default mode
    currentModeInfo = SynthEngine::getModeInfo(0);
    
    // Don't make opaque to allow transparency effects
    setOpaque(false);
    
    // Start animation timer at 60 Hz for smooth motion
    startTimerHz(60);
}

EnhancedVisualizer::~EnhancedVisualizer()
{
    stopTimer();
}

void EnhancedVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Fill background black
    g.fillAll(juce::Colours::black);
    
    // If not playing, fade to black
    if (!isPlaying)
    {
        return;
    }
    
    // Calculate grain positions and colors
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.48f;
    
    // High-resolution grain rendering
    float grainSize = scale * 2.0f / GRID_SIZE;
    
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            const auto& grain = grainField[i][j];
            
            // Skip grains with very low amplitude (nodes)
            if (grain.amplitude < 0.02f) continue;
            
            // Calculate screen position with vibration displacement
            float px = centerX + (grain.x - 0.5f) * scale * 2;
            float py = centerY + (grain.y - 0.5f + grain.displacement * 0.02f) * scale * 2;
            
            // Color based on amplitude and mode
            float colorPos = grain.amplitude * grain.brightness;
            auto color = interpolateColor(colorPos);
            
            // Draw grain with size modulation
            float visualSize = grainSize * 0.8f * grain.size;
            
            g.setColour(color.withAlpha(grain.brightness));
            g.fillEllipse(px - visualSize/2, py - visualSize/2, visualSize, visualSize);
        }
    }
}

void EnhancedVisualizer::resized()
{
    // Update render cache size if needed
    auto size = getLocalBounds();
    if (renderCache.getWidth() != size.getWidth() || renderCache.getHeight() != size.getHeight())
    {
        renderCache = juce::Image(juce::Image::ARGB, size.getWidth(), size.getHeight(), true);
        cacheDirty = true;
    }
}

void EnhancedVisualizer::timerCallback()
{
    // Update time
    const float deltaTime = 1.0f / 60.0f;
    currentTime += deltaTime;
    
    // Update silence timer
    if (!isPlaying)
    {
        silenceTimer += deltaTime;
    }
    else
    {
        silenceTimer = 0.0f;
    }
    
    // Smooth frequency morphing
    if (std::abs(targetFrequency - currentFrequency) > 0.1f)
    {
        currentFrequency += (targetFrequency - currentFrequency) * 0.15f;
        updateModeParameters(currentFrequency);
        updateGrainField();
    }
    
    // Update grain vibration
    updateGrainVibration(deltaTime);
    
    // Trigger repaint
    repaint();
}

void EnhancedVisualizer::setFrequency(float freq)
{
    targetFrequency = freq;
    setActive(freq > 0.0f);
}

void EnhancedVisualizer::setFrequencies(const std::vector<float>& frequencies)
{
    activeFrequencies = frequencies;
    
    if (!frequencies.empty())
    {
        // For polyphonic, use weighted average or dominant frequency
        float avgFreq = 0.0f;
        for (float f : frequencies)
        {
            avgFreq += f;
        }
        targetFrequency = avgFreq / frequencies.size();
        setActive(true);
    }
    else
    {
        setActive(false);
    }
}

void EnhancedVisualizer::setSynthMode(int mode)
{
    currentSynthMode = mode;
    currentModeInfo = SynthEngine::getModeInfo(mode);
    cacheDirty = true;
}

void EnhancedVisualizer::setActive(bool active)
{
    isPlaying = active;
    if (!active)
    {
        // Reset grain vibration when stopping
        for (int i = 0; i < GRID_SIZE; i++)
        {
            for (int j = 0; j < GRID_SIZE; j++)
            {
                grainField[i][j].displacement = 0.0f;
                grainField[i][j].brightness = 0.0f;
            }
        }
    }
}

void EnhancedVisualizer::updateModeParameters(float frequency)
{
    // Calculate Chladni pattern modes based on frequency
    float rank = CymaglyphModes::frequencyToModeRank(frequency);
    auto modePair = CymaglyphModes::getModePair(squareModes, rank);
    
    modeParams.mode1_m = modePair.first.m;
    modeParams.mode1_n = modePair.first.n;
    modeParams.mode2_m = modePair.second.m;
    modeParams.mode2_n = modePair.second.n;
    
    modeParams.modeCrossfade = CymaglyphModes::getModeCrossfade(rank, squareModes.size());
}

void EnhancedVisualizer::updateGrainField()
{
    // Calculate standing wave pattern for current frequency
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            float x = i / float(GRID_SIZE - 1);
            float y = j / float(GRID_SIZE - 1);
            
            // Calculate Chladni pattern amplitude
            float u1 = std::sin(juce::MathConstants<float>::pi * modeParams.mode1_m * x) *
                      std::sin(juce::MathConstants<float>::pi * modeParams.mode1_n * y);
            float u2 = std::sin(juce::MathConstants<float>::pi * modeParams.mode2_m * x) *
                      std::sin(juce::MathConstants<float>::pi * modeParams.mode2_n * y);
            
            float amplitude = std::abs(u1 * (1.0f - modeParams.modeCrossfade) + 
                                     u2 * modeParams.modeCrossfade);
            
            // For polyphonic mode, add interference patterns
            if (activeFrequencies.size() > 1)
            {
                for (size_t k = 1; k < activeFrequencies.size(); k++)
                {
                    float freqRatio = activeFrequencies[k] / activeFrequencies[0];
                    float interference = std::sin(juce::MathConstants<float>::pi * modeParams.mode1_m * x * freqRatio) *
                                       std::sin(juce::MathConstants<float>::pi * modeParams.mode1_n * y * freqRatio);
                    amplitude += std::abs(interference) * 0.5f / activeFrequencies.size();
                }
            }
            
            // Smooth transition
            grainField[i][j].amplitude = grainField[i][j].amplitude * 0.8f + amplitude * 0.2f;
        }
    }
}

void EnhancedVisualizer::updateGrainVibration(float deltaTime)
{
    if (!isPlaying) return;
    
    // Each grain vibrates at the current frequency
    float vibrationSpeed = currentFrequency * 2.0f * juce::MathConstants<float>::pi;
    
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            auto& grain = grainField[i][j];
            
            // Update vibration phase
            grain.phase += vibrationSpeed * deltaTime;
            if (grain.phase > 2.0f * juce::MathConstants<float>::pi)
                grain.phase -= 2.0f * juce::MathConstants<float>::pi;
            
            // Calculate displacement based on amplitude and phase
            grain.displacement = grain.amplitude * std::sin(grain.phase);
            
            // Size pulsing with frequency
            grain.size = 1.0f + 0.2f * grain.displacement;
            
            // Brightness modulation
            grain.brightness = 0.7f + 0.3f * grain.amplitude * (1.0f + std::sin(grain.phase));
            
            // For high frequencies, add shimmer effect
            if (currentFrequency > 1000.0f)
            {
                grain.brightness *= (0.9f + 0.1f * std::sin(grain.phase * 7.0f));
            }
        }
    }
}

juce::Colour EnhancedVisualizer::interpolateColor(float position)
{
    // Interpolate between primary and secondary colors based on position
    position = juce::jlimit(0.0f, 1.0f, position);
    
    const auto& primary = currentModeInfo.primaryColor;
    const auto& secondary = currentModeInfo.secondaryColor;
    const auto& accent = currentModeInfo.accentColor;
    
    if (position < 0.5f)
    {
        // Interpolate between primary and secondary
        float t = position * 2.0f;
        return primary.interpolatedWith(secondary, t);
    }
    else
    {
        // Interpolate between secondary and accent
        float t = (position - 0.5f) * 2.0f;
        return secondary.interpolatedWith(accent, t);
    }
}