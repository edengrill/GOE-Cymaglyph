#include "Visualizer.h"
#include "ShaderPrograms.h"
#include <cmath>

CymaglyphVisualizer::CymaglyphVisualizer(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
    // Initialize mode tables
    squareModes = CymaglyphModes::getSquareModes();
    circularModes = CymaglyphModes::getCircularModes();
    
    // Make component opaque for faster rendering
    setOpaque(true);
    
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
    // Fill background with test pattern to verify painting is working
    g.fillAll(juce::Colour::fromHSV(currentTime * 0.1f, 0.5f, 0.5f, 1.0f));
    
    // Get current parameters
    const float freq = targetFrequency.load();
    const int medium = static_cast<int>(parameters.getRawParameterValue("medium")->load());
    const int geom = static_cast<int>(parameters.getRawParameterValue("geom")->load());
    const float nodeEps = parameters.getRawParameterValue("nodeEps")->load();
    const float grainAmt = parameters.getRawParameterValue("grainAmt")->load();
    const int colorMode = static_cast<int>(parameters.getRawParameterValue("colorMode")->load());
    
    auto bounds = getLocalBounds();
    float centerX = bounds.getCentreX();
    float centerY = bounds.getCentreY();
    float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;
    
    // Update mode parameters based on frequency
    updateModeParameters(freq);
    
    // Create cymatics pattern
    if (medium == 2) // Water/Faraday
    {
        // Draw water ripples
        for (int ring = 0; ring < 8; ring++)
        {
            float ringRadius = scale * (0.2f + ring * 0.1f);
            float wave = std::sin(currentTime * 3.0f + ring * 0.5f);
            float actualRadius = ringRadius + wave * 10.0f;
            
            float intensity = 1.0f - (ring * 0.12f);
            auto color = colorMode == 1 ? 
                juce::Colour::fromHSV(0.6f - ring * 0.1f, 0.8f, intensity, 1.0f) :
                juce::Colour::fromFloatRGBA(intensity, intensity, intensity, 1.0f);
            
            g.setColour(color);
            g.drawEllipse(centerX - actualRadius, centerY - actualRadius,
                         actualRadius * 2, actualRadius * 2, 2.0f);
            
            // Add rotating spokes
            int numSpokes = modeParams.water_n;
            for (int spoke = 0; spoke < numSpokes; spoke++)
            {
                float angle = (spoke / float(numSpokes)) * 2.0f * 3.14159f + currentTime;
                float x1 = centerX + actualRadius * 0.5f * std::cos(angle);
                float y1 = centerY + actualRadius * 0.5f * std::sin(angle);
                float x2 = centerX + actualRadius * std::cos(angle);
                float y2 = centerY + actualRadius * std::sin(angle);
                
                g.drawLine(x1, y1, x2, y2, 1.0f);
            }
        }
    }
    else if (geom == 0) // Square
    {
        // Draw square plate modes
        int gridSize = 32;
        float cellSize = (scale * 2) / gridSize;
        
        for (int i = 0; i < gridSize; i++)
        {
            for (int j = 0; j < gridSize; j++)
            {
                float x = (i / float(gridSize - 1)) - 0.5f;
                float y = (j / float(gridSize - 1)) - 0.5f;
                
                // Calculate mode amplitude
                float u1 = std::sin(3.14159f * modeParams.mode1_m * (x + 0.5f)) *
                          std::sin(3.14159f * modeParams.mode1_n * (y + 0.5f));
                float u2 = std::sin(3.14159f * modeParams.mode2_m * (x + 0.5f)) *
                          std::sin(3.14159f * modeParams.mode2_n * (y + 0.5f));
                
                float amplitude = juce::jlimit(-1.0f, 1.0f,
                    (u1 * (1.0f - modeParams.modeCrossfade) + u2 * modeParams.modeCrossfade) *
                    std::cos(currentTime * 5.0f));
                
                float intensity = std::abs(amplitude);
                
                if (intensity > nodeEps)
                {
                    auto color = colorMode == 1 ?
                        juce::Colour::fromHSV(0.6f - intensity * 0.6f, 0.8f, intensity, 1.0f) :
                        juce::Colour::fromFloatRGBA(intensity, intensity, intensity, 1.0f);
                    
                    float px = centerX + x * scale * 2;
                    float py = centerY + y * scale * 2;
                    
                    g.setColour(color);
                    g.fillEllipse(px - cellSize/2, py - cellSize/2, cellSize, cellSize);
                }
            }
        }
    }
    else // Circle
    {
        // Draw circular membrane modes
        int numRings = 20;
        int numAngles = 64;
        
        for (int r = 0; r < numRings; r++)
        {
            float radius = (r / float(numRings)) * scale;
            
            g.setColour(juce::Colours::cyan.withAlpha(0.3f));
            
            juce::Path path;
            bool firstPoint = true;
            
            for (int a = 0; a <= numAngles; a++)
            {
                float angle = (a / float(numAngles)) * 2.0f * 3.14159f;
                
                // Simple circular pattern based on mode
                float modulation = std::sin(modeParams.mode1_n * angle + currentTime * 2.0f) *
                                  std::sin(radius * 0.05f * modeParams.mode1_alpha);
                
                float actualRadius = radius + modulation * 20.0f;
                float x = centerX + actualRadius * std::cos(angle);
                float y = centerY + actualRadius * std::sin(angle);
                
                if (firstPoint)
                {
                    path.startNewSubPath(x, y);
                    firstPoint = false;
                }
                else
                {
                    path.lineTo(x, y);
                }
            }
            
            g.strokePath(path, juce::PathStrokeType(1.0f));
        }
    }
    
    // Draw accumulation buffer with grain effect
    if (grainAmt > 0.01f)
    {
        g.setOpacity(grainAmt);
        g.drawImageAt(accumBuffer, 0, 0);
    }
    
    // Update accumulation buffer
    updateAccumulationBuffer();
    
    // Draw frequency text
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(14.0f);
    g.drawText(juce::String(freq, 1) + " Hz", bounds.removeFromTop(20), juce::Justification::centred);
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
    const int geom = static_cast<int>(parameters.getRawParameterValue("geom")->load());
    const int mount = static_cast<int>(parameters.getRawParameterValue("mount")->load());
    const int medium = static_cast<int>(parameters.getRawParameterValue("medium")->load());
    
    if (medium == 2) // Water
    {
        auto waterMode = CymaglyphModes::getWaterMode(frequency);
        modeParams.water_n = waterMode.n;
        modeParams.water_k1 = waterMode.k1;
        modeParams.water_k2 = waterMode.k2;
        modeParams.water_amp1 = waterMode.amp1;
        modeParams.water_amp2 = waterMode.amp2;
    }
    else if (geom == 0) // Square
    {
        float rank = CymaglyphModes::frequencyToModeRank(frequency);
        auto modePair = CymaglyphModes::getModePair(squareModes, rank);
        
        modeParams.mode1_m = modePair.first.m;
        modeParams.mode1_n = modePair.first.n;
        modeParams.mode2_m = modePair.second.m;
        modeParams.mode2_n = modePair.second.n;
        
        modeParams.modeCrossfade = CymaglyphModes::getModeCrossfade(rank, squareModes.size());
        
        // Apply mounting weights
        modeParams.mode1_weight = CymaglyphModes::getCenterClampWeight(
            modePair.first.m, modePair.first.n, mount == 1);
        modeParams.mode2_weight = CymaglyphModes::getCenterClampWeight(
            modePair.second.m, modePair.second.n, mount == 1);
    }
    else // Circle
    {
        float rank = CymaglyphModes::frequencyToModeRank(frequency);
        auto modePair = CymaglyphModes::getModePair(circularModes, rank);
        
        modeParams.mode1_n = modePair.first.n;
        modeParams.mode1_alpha = modePair.first.alpha;
        modeParams.mode2_n = modePair.second.n;
        modeParams.mode2_alpha = modePair.second.alpha;
        
        modeParams.modeCrossfade = CymaglyphModes::getModeCrossfade(rank, circularModes.size());
        modeParams.mode1_weight = 1.0f;
        modeParams.mode2_weight = 1.0f;
    }
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