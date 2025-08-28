#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModeTables.h"
#include "SynthEngine.h"
#include <atomic>
#include <vector>

class EnhancedVisualizer : public juce::Component,
                           private juce::Timer
{
public:
    EnhancedVisualizer(juce::AudioProcessorValueTreeState& apvts);
    ~EnhancedVisualizer() override;
    
    // Component
    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Public interface
    void setFrequency(float freq);
    void setFrequencies(const std::vector<float>& frequencies);
    void setSynthMode(int mode);
    void setActive(bool active);
    
    // Get current state
    bool isActive() const { return isPlaying; }
    
private:
    // Timer callback for animation
    void timerCallback() override;
    
    // Grain structure for high-res visualization
    struct Grain
    {
        float x, y;                  // Base position
        float amplitude;             // Standing wave amplitude
        float phase;                 // Current vibration phase
        float displacement;          // Current displacement from base
        float size;                  // Visual size
        float brightness;            // Current brightness
        juce::Colour color;          // Current color
    };
    
    // Visualization parameters
    static constexpr int GRID_SIZE = 96;
    std::array<std::array<Grain, GRID_SIZE>, GRID_SIZE> grainField;
    
    // Mode calculation
    void updateModeParameters(float frequency);
    void updateGrainField();
    void updateGrainVibration(float deltaTime);
    
    // Morphing system
    float morphProgress = 0.0f;
    float targetFrequency = 440.0f;
    float currentFrequency = 440.0f;
    std::vector<float> activeFrequencies;
    
    // Color palette
    SynthEngine::ModeInfo currentModeInfo;
    int currentSynthMode = 0;
    juce::Colour interpolateColor(float position);
    
    // Animation state
    float currentTime = 0.0f;
    bool isPlaying = false;
    float silenceTimer = 0.0f;
    
    // Parameters
    juce::AudioProcessorValueTreeState& parameters;
    
    // Mode data
    std::vector<CymaglyphModes::SquareMode> squareModes;
    
    // Current mode parameters
    struct ModeParams {
        int mode1_m = 1, mode1_n = 1;
        int mode2_m = 2, mode2_n = 1;
        float modeCrossfade = 0.0f;
    };
    ModeParams modeParams;
    
    // Performance optimization
    juce::Image renderCache;
    bool cacheDirty = true;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnhancedVisualizer)
};