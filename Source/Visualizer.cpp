#include "Visualizer.h"
#include "ShaderPrograms.h"
#include <cmath>

CymaglyphVisualizer::CymaglyphVisualizer(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
    // Initialize mode tables
    squareModes = CymaglyphModes::getSquareModes();
    circularModes = CymaglyphModes::getCircularModes();
    
    // Make component non-opaque to ensure it renders
    setOpaque(false);
    
    // Initialize accumulation buffer
    accumBuffer = juce::Image(juce::Image::ARGB, 512, 512, true);
    accumBuffer.clear(accumBuffer.getBounds(), juce::Colours::black);
    
    // Start timer for animation at 30 Hz (CPU rendering is slower)
    startTimerHz(30);
}

CymaglyphVisualizer::~CymaglyphVisualizer()
{
    stopTimer();
}

void CymaglyphVisualizer::paint(juce::Graphics& g)
{
    // Fill background black
    auto bounds = getLocalBounds();
    g.fillAll(juce::Colours::black);
    
    // Get current parameters
    const float freq = targetFrequency.load();
    const float nodeEps = 0.02f; // Fixed threshold
    const float grainAmt = parameters.getRawParameterValue("grainAmt")->load();
    
    // Use already declared bounds
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;
    
    // Update mode parameters based on frequency
    updateModeParameters(freq);
    
    // Draw square Chladni pattern
    int gridSize = 48; // Higher resolution
    float cellSize = (scale * 2) / gridSize;
    
    for (int i = 0; i < gridSize; i++)
    {
        for (int j = 0; j < gridSize; j++)
        {
            float x = (i / float(gridSize - 1)) - 0.5f;
            float y = (j / float(gridSize - 1)) - 0.5f;
            
            // Calculate mode amplitude (no pulsing)
            float u1 = std::sin(3.14159f * modeParams.mode1_m * (x + 0.5f)) *
                      std::sin(3.14159f * modeParams.mode1_n * (y + 0.5f));
            float u2 = std::sin(3.14159f * modeParams.mode2_m * (x + 0.5f)) *
                      std::sin(3.14159f * modeParams.mode2_n * (y + 0.5f));
            
            float amplitude = juce::jlimit(-1.0f, 1.0f,
                (u1 * (1.0f - modeParams.modeCrossfade) + u2 * modeParams.modeCrossfade));
            
            float intensity = std::abs(amplitude);
            
            if (intensity > nodeEps)
            {
                // Simple cyan-to-magenta gradient based on intensity
                auto color = juce::Colour::fromHSV(
                    0.5f + intensity * 0.33f,  // Hue from cyan to magenta
                    0.7f + intensity * 0.3f,    // Saturation
                    intensity,                   // Brightness
                    1.0f);
                
                float px = centerX + x * scale * 2;
                float py = centerY + y * scale * 2;
                
                g.setColour(color);
                g.fillEllipse(px - cellSize/2, py - cellSize/2, cellSize, cellSize);
            }
        }
    }
    
    // Draw accumulation buffer with grain effect
    if (grainAmt > 0.01f)
    {
        g.setOpacity(grainAmt * 0.3f); // Reduced opacity
        g.drawImageAt(accumBuffer, 0, 0);
    }
    
    // Update accumulation buffer
    updateAccumulationBuffer();
}

void CymaglyphVisualizer::resized()
{
    // Recreate accumulation buffer at new size
    const int size = juce::jmin(getWidth(), getHeight());
    if (size > 0 && (accumBuffer.getWidth() != size || accumBuffer.getHeight() != size))
    {
        juce::ScopedLock lock(accumBufferLock);
        accumBuffer = juce::Image(juce::Image::ARGB, size, size, true);
        accumBuffer.clear(accumBuffer.getBounds(), juce::Colours::black);
        accumBufferDirty = true;
    }
}

void CymaglyphVisualizer::timerCallback()
{
    // Update time
    currentTime += 1.0f / 30.0f;
    
    // Update mode parameters based on frequency
    updateModeParameters(targetFrequency.load());
    
    // Trigger repaint
    repaint();
}

void CymaglyphVisualizer::updateModeParameters(float frequency)
{
    // Always use square modes for v2
    float rank = CymaglyphModes::frequencyToModeRank(frequency);
    auto modePair = CymaglyphModes::getModePair(squareModes, rank);
    
    modeParams.mode1_m = modePair.first.m;
    modeParams.mode1_n = modePair.first.n;
    modeParams.mode2_m = modePair.second.m;
    modeParams.mode2_n = modePair.second.n;
    
    modeParams.modeCrossfade = CymaglyphModes::getModeCrossfade(rank, squareModes.size());
    
    // Edge clamped by default
    modeParams.mode1_weight = 1.0f;
    modeParams.mode2_weight = 1.0f;
}

void CymaglyphVisualizer::updateAccumulationBuffer()
{
    juce::ScopedLock lock(accumBufferLock);
    
    // Apply decay to existing accumulation
    juce::Image::BitmapData bitmap(accumBuffer, juce::Image::BitmapData::readWrite);
    
    for (int y = 0; y < bitmap.height; ++y)
    {
        for (int x = 0; x < bitmap.width; ++x)
        {
            auto* pixel = bitmap.getPixelPointer(x, y);
            pixel[0] = static_cast<uint8_t>(pixel[0] * accumDecay); // R
            pixel[1] = static_cast<uint8_t>(pixel[1] * accumDecay); // G
            pixel[2] = static_cast<uint8_t>(pixel[2] * accumDecay); // B
        }
    }
    
    accumBufferDirty = true;
}

void CymaglyphVisualizer::resetAccumulation()
{
    juce::ScopedLock lock(accumBufferLock);
    accumBuffer.clear(accumBuffer.getBounds(), juce::Colours::black);
    accumBufferDirty = true;
}

void CymaglyphVisualizer::saveImage(const juce::File& file)
{
    juce::ScopedLock lock(accumBufferLock);
    
    juce::PNGImageFormat pngFormat;
    juce::FileOutputStream stream(file);
    
    if (stream.openedOk())
    {
        pngFormat.writeImageToStream(accumBuffer, stream);
    }
}

// Stub functions for removed OpenGL methods
void CymaglyphVisualizer::newOpenGLContextCreated() {}
void CymaglyphVisualizer::renderOpenGL() {}
void CymaglyphVisualizer::openGLContextClosing() {}
void CymaglyphVisualizer::createShaders() {}
void CymaglyphVisualizer::createQuad() {}
void CymaglyphVisualizer::updateUniforms() {}
void CymaglyphVisualizer::uploadAccumulationTexture() {}