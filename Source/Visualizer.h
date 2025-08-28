#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ModeTables.h"
#include <atomic>

class CymaglyphVisualizer : public juce::Component,
                            public juce::OpenGLRenderer,
                            private juce::Timer
{
public:
    CymaglyphVisualizer(juce::AudioProcessorValueTreeState& apvts);
    ~CymaglyphVisualizer() override;
    
    // Component
    void paint(juce::Graphics&) override;
    void resized() override;
    
    // OpenGLRenderer
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    
    // Public interface
    void resetAccumulation();
    void saveImage(const juce::File& file);
    void setFrequency(float freq) { targetFrequency = freq; }
    
private:
    // Timer callback
    void timerCallback() override;
    
    // OpenGL setup
    void createShaders();
    void createQuad();
    void updateUniforms();
    
    // Accumulation buffer management
    void updateAccumulationBuffer();
    void uploadAccumulationTexture();
    
    // Mode calculation
    void updateModeParameters(float frequency);
    
    // OpenGL resources
    juce::OpenGLContext openGLContext;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformResolution;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformTime;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformFreq;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformNodeEps;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMedium;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformGeom;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMount;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformAccuracy;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformAccumTex;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformAccumMix;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformGrainAmt;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformColorMode;
    
    // Mode uniforms
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode1_m;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode1_n;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode2_m;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode2_n;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode1_alpha;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode2_alpha;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformModeCrossfade;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode1_weight;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformMode2_weight;
    
    // Water mode uniforms
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformWater_n;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformWater_k1;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformWater_k2;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformWater_amp1;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uniformWater_amp2;
    
    // Vertex buffer
    GLuint vbo = 0;
    GLuint vao = 0;
    
    // Textures
    GLuint accumTexture = 0;
    
    // Accumulation buffer
    juce::Image accumBuffer;
    juce::CriticalSection accumBufferLock;
    std::atomic<bool> accumBufferDirty{false};
    float accumDecay = 0.98f;
    float accumAlpha = 0.1f;
    
    // Parameters
    juce::AudioProcessorValueTreeState& parameters;
    std::atomic<float> targetFrequency{440.0f};
    float currentTime = 0.0f;
    
    // Mode data
    std::vector<CymaglyphModes::SquareMode> squareModes;
    std::vector<CymaglyphModes::CircularMode> circularModes;
    
    // Current mode parameters
    struct ModeParams {
        // Square/Circle modes
        int mode1_m = 1, mode1_n = 1;
        int mode2_m = 2, mode2_n = 1;
        float mode1_alpha = 2.4048f;
        float mode2_alpha = 3.8317f;
        float modeCrossfade = 0.0f;
        float mode1_weight = 1.0f;
        float mode2_weight = 1.0f;
        
        // Water modes
        int water_n = 3;
        float water_k1 = 6.0f;
        float water_k2 = 10.0f;
        float water_amp1 = 1.0f;
        float water_amp2 = 0.6f;
    };
    ModeParams modeParams;
    
    // Frame counter for performance
    int frameCounter = 0;
    double lastFpsTime = 0.0;
    float currentFps = 60.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CymaglyphVisualizer)
};