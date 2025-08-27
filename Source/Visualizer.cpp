#include "Visualizer.h"
#include "ShaderPrograms.h"

CymaglyphVisualizer::CymaglyphVisualizer(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
    // Initialize mode tables
    squareModes = CymaglyphModes::getSquareModes();
    circularModes = CymaglyphModes::getCircularModes();
    
    // Setup OpenGL
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    openGLContext.setContinuousRepainting(true);
    
    // Initialize accumulation buffer (will be resized properly later)
    accumBuffer = juce::Image(juce::Image::ARGB, 1024, 1024, true);
    
    // Start timer for animation at 60 Hz
    startTimerHz(60);
}

CymaglyphVisualizer::~CymaglyphVisualizer()
{
    stopTimer();
    openGLContext.detach();
}

void CymaglyphVisualizer::paint(juce::Graphics& g)
{
    // OpenGL handles the rendering
    g.fillAll(juce::Colours::black);
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

void CymaglyphVisualizer::newOpenGLContextCreated()
{
    createShaders();
    createQuad();
    
    // Create accumulation texture
    glGenTextures(1, &accumTexture);
    glBindTexture(GL_TEXTURE_2D, accumTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void CymaglyphVisualizer::openGLContextClosing()
{
    shader.reset();
    
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    
    if (accumTexture != 0) {
        glDeleteTextures(1, &accumTexture);
        accumTexture = 0;
    }
}

void CymaglyphVisualizer::renderOpenGL()
{
    using namespace juce::gl;
    
    const auto bounds = getLocalBounds();
    glViewport(0, 0, bounds.getWidth(), bounds.getHeight());
    
    // Clear
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (shader == nullptr || !shader->getUniformIDFromName("uRes"))
        return;
    
    shader->use();
    
    // Update uniforms
    updateUniforms();
    
    // Upload accumulation texture if needed
    if (accumBufferDirty.exchange(false))
    {
        uploadAccumulationTexture();
    }
    
    // Bind accumulation texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumTexture);
    
    // Draw quad
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    
    // Update accumulation buffer (on message thread via timer)
    // This is handled in timerCallback()
    
    // FPS counter
    frameCounter++;
    const double currentTimeMs = juce::Time::getMillisecondCounterHiRes();
    if (currentTimeMs - lastFpsTime > 1000.0)
    {
        currentFps = frameCounter * 1000.0f / (currentTimeMs - lastFpsTime);
        frameCounter = 0;
        lastFpsTime = currentTimeMs;
    }
}

void CymaglyphVisualizer::timerCallback()
{
    // Update time
    currentTime += 1.0f / 60.0f;
    
    // Update mode parameters based on frequency
    updateModeParameters(targetFrequency.load());
    
    // Update accumulation buffer
    updateAccumulationBuffer();
    
    // Trigger repaint
    openGLContext.triggerRepaint();
}

void CymaglyphVisualizer::createShaders()
{
    shader.reset(new juce::OpenGLShaderProgram(openGLContext));
    
    if (!shader->addVertexShader(ShaderPrograms::kVertexShader))
    {
        DBG("Vertex shader compile error: " << shader->getLastError());
        return;
    }
    
    if (!shader->addFragmentShader(ShaderPrograms::kFragmentCymaglyph))
    {
        DBG("Fragment shader compile error: " << shader->getLastError());
        return;
    }
    
    if (!shader->link())
    {
        DBG("Shader link error: " << shader->getLastError());
        return;
    }
    
    shader->use();
    
    // Cache uniform locations
    uniformResolution.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uRes"));
    uniformTime.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uTime"));
    uniformFreq.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uFreq"));
    uniformNodeEps.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uNodeEps"));
    uniformMedium.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMedium"));
    uniformGeom.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uGeom"));
    uniformMount.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMount"));
    uniformAccuracy.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uAccuracy"));
    uniformAccumTex.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uAccum"));
    uniformAccumMix.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uAccumMix"));
    uniformGrainAmt.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uGrainAmt"));
    uniformColorMode.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uColorMode"));
    
    // Mode uniforms
    uniformMode1_m.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode1_m"));
    uniformMode1_n.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode1_n"));
    uniformMode2_m.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode2_m"));
    uniformMode2_n.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode2_n"));
    uniformMode1_alpha.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode1_alpha"));
    uniformMode2_alpha.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode2_alpha"));
    uniformModeCrossfade.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uModeCrossfade"));
    uniformMode1_weight.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode1_weight"));
    uniformMode2_weight.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uMode2_weight"));
    
    // Water uniforms
    uniformWater_n.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uWater_n"));
    uniformWater_k1.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uWater_k1"));
    uniformWater_k2.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uWater_k2"));
    uniformWater_amp1.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uWater_amp1"));
    uniformWater_amp2.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "uWater_amp2"));
}

void CymaglyphVisualizer::createQuad()
{
    // Full screen quad vertices
    const GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    const GLuint positionAttribute = 0;
    glEnableVertexAttribArray(positionAttribute);
    glVertexAttribPointer(positionAttribute, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    
    glBindVertexArray(0);
}

void CymaglyphVisualizer::updateUniforms()
{
    if (!shader) return;
    
    // Resolution
    if (uniformResolution != nullptr)
        uniformResolution->set((GLfloat)getWidth(), (GLfloat)getHeight());
    
    // Time
    if (uniformTime != nullptr)
        uniformTime->set(currentTime);
    
    // Frequency
    if (uniformFreq != nullptr)
        uniformFreq->set(targetFrequency.load());
    
    // Visual parameters
    const float nodeEps = parameters.getRawParameterValue("nodeEps")->load();
    const int medium = static_cast<int>(parameters.getRawParameterValue("medium")->load());
    const int geom = static_cast<int>(parameters.getRawParameterValue("geom")->load());
    const int mount = static_cast<int>(parameters.getRawParameterValue("mount")->load());
    const float accuracy = parameters.getRawParameterValue("accuracy")->load();
    const float grainAmt = parameters.getRawParameterValue("grainAmt")->load();
    const int colorMode = static_cast<int>(parameters.getRawParameterValue("colorMode")->load());
    
    if (uniformNodeEps != nullptr) uniformNodeEps->set(nodeEps);
    if (uniformMedium != nullptr) uniformMedium->set(medium);
    if (uniformGeom != nullptr) uniformGeom->set(geom);
    if (uniformMount != nullptr) uniformMount->set(mount);
    if (uniformAccuracy != nullptr) uniformAccuracy->set(accuracy);
    if (uniformGrainAmt != nullptr) uniformGrainAmt->set(grainAmt);
    if (uniformColorMode != nullptr) uniformColorMode->set(colorMode);
    
    // Accumulation mixing
    if (uniformAccumMix != nullptr) uniformAccumMix->set(accumAlpha);
    if (uniformAccumTex != nullptr) uniformAccumTex->set(0); // Texture unit 0
    
    // Mode-specific uniforms
    if (uniformMode1_m != nullptr) uniformMode1_m->set(modeParams.mode1_m);
    if (uniformMode1_n != nullptr) uniformMode1_n->set(modeParams.mode1_n);
    if (uniformMode2_m != nullptr) uniformMode2_m->set(modeParams.mode2_m);
    if (uniformMode2_n != nullptr) uniformMode2_n->set(modeParams.mode2_n);
    if (uniformMode1_alpha != nullptr) uniformMode1_alpha->set(modeParams.mode1_alpha);
    if (uniformMode2_alpha != nullptr) uniformMode2_alpha->set(modeParams.mode2_alpha);
    if (uniformModeCrossfade != nullptr) uniformModeCrossfade->set(modeParams.modeCrossfade);
    if (uniformMode1_weight != nullptr) uniformMode1_weight->set(modeParams.mode1_weight);
    if (uniformMode2_weight != nullptr) uniformMode2_weight->set(modeParams.mode2_weight);
    
    // Water mode uniforms
    if (uniformWater_n != nullptr) uniformWater_n->set(modeParams.water_n);
    if (uniformWater_k1 != nullptr) uniformWater_k1->set(modeParams.water_k1);
    if (uniformWater_k2 != nullptr) uniformWater_k2->set(modeParams.water_k2);
    if (uniformWater_amp1 != nullptr) uniformWater_amp1->set(modeParams.water_amp1);
    if (uniformWater_amp2 != nullptr) uniformWater_amp2->set(modeParams.water_amp2);
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

void CymaglyphVisualizer::uploadAccumulationTexture()
{
    juce::ScopedLock lock(accumBufferLock);
    
    if (accumTexture != 0 && accumBuffer.isValid())
    {
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        
        juce::Image::BitmapData bitmap(accumBuffer, juce::Image::BitmapData::readOnly);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                    bitmap.width, bitmap.height, 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, bitmap.data);
    }
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