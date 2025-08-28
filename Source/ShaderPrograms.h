#pragma once

namespace ShaderPrograms {

const char* kVertexShader = R"GLSL(
#version 120
attribute vec4 position;
varying vec2 texCoord;

void main() {
    gl_Position = position;
    texCoord = position.xy * 0.5 + 0.5;
}
)GLSL";

const char* kFragmentCymaglyph = R"GLSL(
#version 120

uniform vec2  uRes;
uniform float uTime;
uniform float uFreq;
uniform float uNodeEps;
uniform int   uMedium;      // 0=Plate,1=Membrane,2=Water
uniform int   uGeom;        // 0=Square,1=Circle
uniform int   uMount;       // 0=EdgeClamped,1=CenterClamped
uniform float uAccuracy;
uniform sampler2D uAccum;
uniform float uAccumMix;
uniform float uGrainAmt;
uniform int   uColorMode;   // 0=Mono,1=Heat

// Mode parameters passed from CPU
uniform int   uMode1_m, uMode1_n;
uniform int   uMode2_m, uMode2_n;
uniform float uMode1_alpha, uMode2_alpha;
uniform float uModeCrossfade;
uniform float uMode1_weight, uMode2_weight;

// Water mode parameters
uniform int   uWater_n;
uniform float uWater_k1, uWater_k2;
uniform float uWater_amp1, uWater_amp2;

varying vec2 texCoord;

const float PI = 3.14159265359;
const float TAU = 6.28318530718;

// Square plate/membrane mode
float squareMode(vec2 uv, int m, int n) {
    return sin(PI * float(m) * uv.x) * sin(PI * float(n) * uv.y);
}

// Cheap Bessel J approximations
float besselJ0(float x) {
    float x2 = x * x;
    return 1.0 - x2/4.0 + x2*x2/64.0 - x2*x2*x2/2304.0;
}

float besselJ1(float x) {
    float x2 = x * x;
    return x/2.0 * (1.0 - x2/8.0 + x2*x2/192.0);
}

float besselJ2(float x) {
    float x2 = x * x;
    return x2/8.0 * (1.0 - x2/12.0 + x2*x2/384.0);
}

float besselJ3(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    return x3/48.0 * (1.0 - x2/16.0);
}

float besselJn(int n, float x) {
    if (n == 0) return besselJ0(x);
    else if (n == 1) return besselJ1(x);
    else if (n == 2) return besselJ2(x);
    else if (n == 3) return besselJ3(x);
    else { // Higher order approximation
        float x2 = x * x;
        return pow(x/2.0, float(n)) / float(n) * (1.0 - x2/(4.0*float(n+1)));
    }
}

// Circular membrane mode
float circularMode(float r, float theta, int n, float alpha) {
    if (r > 1.0) return 0.0;
    return besselJn(n, alpha * r) * cos(float(n) * theta);
}

vec3 heatColorMap(float t) {
    // Blue -> Cyan -> Green -> Yellow -> Red
    t = clamp(t, 0.0, 1.0);
    vec3 c1 = vec3(0.0, 0.0, 1.0);  // Blue
    vec3 c2 = vec3(0.0, 1.0, 1.0);  // Cyan
    vec3 c3 = vec3(0.0, 1.0, 0.0);  // Green
    vec3 c4 = vec3(1.0, 1.0, 0.0);  // Yellow
    vec3 c5 = vec3(1.0, 0.0, 0.0);  // Red
    
    float stage = t * 4.0;
    if (stage < 1.0) return mix(c1, c2, stage);
    else if (stage < 2.0) return mix(c2, c3, stage - 1.0);
    else if (stage < 3.0) return mix(c3, c4, stage - 2.0);
    else return mix(c4, c5, stage - 3.0);
}

void main() {
    vec2 uv = texCoord;
    float nodeMask = 0.0;
    float waveValue = 0.0;
    
    if (uMedium == 2) {
        // Water/Faraday patterns
        vec2 centered = uv * 2.0 - 1.0;
        float r = length(centered);
        float theta = atan(centered.y, centered.x);
        
        if (r <= 1.0) {
            float wave1 = cos(float(uWater_n) * theta) * sin(TAU * r * uWater_k1);
            float wave2 = cos(2.0 * float(uWater_n) * theta) * sin(TAU * r * uWater_k2);
            waveValue = sin(TAU * (uFreq * 0.5) * uTime) * 
                       (uWater_amp1 * wave1 + uWater_amp2 * wave2);
            
            // Create ripple-like node pattern
            nodeMask = smoothstep(uNodeEps, 0.0, abs(waveValue));
        }
    }
    else if (uGeom == 0) {
        // Square plate/membrane
        float mode1 = squareMode(uv, uMode1_m, uMode1_n) * uMode1_weight;
        float mode2 = squareMode(uv, uMode2_m, uMode2_n) * uMode2_weight;
        waveValue = mix(mode1, mode2, uModeCrossfade) * cos(TAU * uTime);
        nodeMask = smoothstep(uNodeEps, 0.0, abs(waveValue));
    }
    else {
        // Circular membrane
        vec2 centered = uv * 2.0 - 1.0;
        float r = length(centered);
        float theta = atan(centered.y, centered.x);
        
        if (r <= 1.0) {
            float mode1 = circularMode(r, theta, uMode1_n, uMode1_alpha) * uMode1_weight;
            float mode2 = circularMode(r, theta, uMode2_n, uMode2_alpha) * uMode2_weight;
            waveValue = mix(mode1, mode2, uModeCrossfade) * cos(TAU * uTime);
            nodeMask = smoothstep(uNodeEps, 0.0, abs(waveValue));
        }
    }
    
    // Sample accumulation buffer
    vec4 accum = texture(uAccum, uv);
    
    // Mix with grain amount
    float grainMask = mix(nodeMask, accum.r, uGrainAmt);
    vec3 finalColor = mix(accum.rgb, vec3(grainMask), uAccumMix);
    
    // Apply color mode
    if (uColorMode == 1) {
        finalColor = heatColorMap(finalColor.r);
    }
    
    gl_FragColor = vec4(finalColor, 1.0);
}
)GLSL";

// Simplified mobile shader
const char* kFragmentCymaglyphMobile = R"GLSL(
#version 100
precision mediump float;

uniform vec2  uRes;
uniform float uTime;
uniform float uFreq;
uniform float uNodeEps;
uniform int   uMedium;
uniform int   uGeom;
uniform sampler2D uAccum;
uniform float uAccumMix;

varying vec2 texCoord;

const float PI = 3.14159265359;
const float TAU = 6.28318530718;

float squareMode(vec2 uv, int m, int n) {
    return sin(PI * float(m) * uv.x) * sin(PI * float(n) * uv.y);
}

void main() {
    vec2 uv = texCoord;
    float nodeMask = 0.0;
    
    if (uMedium == 2) {
        // Simplified water pattern
        vec2 centered = uv * 2.0 - 1.0;
        float r = length(centered);
        float theta = atan(centered.y, centered.x);
        float n = 3.0;
        float waveValue = sin(TAU * uFreq * 0.5 * uTime) * 
                         cos(n * theta) * sin(TAU * r * 6.0);
        nodeMask = smoothstep(uNodeEps, 0.0, abs(waveValue));
    }
    else {
        // Simplified square mode
        float k = (log2(uFreq / 27.5)) / (log2(3520.0 / 27.5));
        int m = 1 + int(k * 4.0);
        int n = 1 + int((1.0 - k) * 4.0);
        float waveValue = squareMode(uv, m, n) * cos(TAU * uTime);
        nodeMask = smoothstep(uNodeEps, 0.0, abs(waveValue));
    }
    
    vec4 accum = texture2D(uAccum, uv);
    vec3 finalColor = mix(accum.rgb, vec3(nodeMask), uAccumMix);
    
    gl_FragColor = vec4(finalColor, 1.0);
}
)GLSL";

} // namespace ShaderPrograms