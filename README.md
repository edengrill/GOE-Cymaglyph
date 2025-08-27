# GOE Cymaglyph

A cross-platform VST3/AUv3 audio plugin that generates tones with real-time cymatics-inspired visualizations.

## Features

- **Tone Generation**: Sine, triangle, and square waveforms with frequency control (20Hz-20kHz)
- **MIDI Support**: Note input sets frequency, with adjustable A4 reference tuning
- **Frequency Sweep**: Automated frequency modulation with adjustable rate and range
- **Cymatics Visualization**: Real-time OpenGL rendering of standing wave patterns
  - Three mediums: Plate, Membrane, Water/Faraday
  - Two geometries: Square, Circle
  - Edge/Center clamped mounting options
  - Continuous mode morphing based on frequency
  - Accumulation buffer for grain/sand settling effects
  - Mono and heat color modes
- **Preset System**: 8 included presets showcasing different visual modes
- **Image Export**: Save current visualization as PNG

## Prerequisites

### macOS
- Xcode 15+ or Command Line Tools
- CMake 3.22+
- macOS 10.13+

### Windows
- Visual Studio 2022
- CMake 3.22+
- Windows 10/11

### Linux
- GCC 11+ or Clang 14+
- CMake 3.22+
- OpenGL development libraries
- ALSA/Jack development libraries

## Build Instructions

### macOS

```bash
# Clone the repository
git clone https://github.com/gardenofed/goe-cymaglyph.git
cd goe-cymaglyph

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# The plugin will be at:
# VST3: build/GOECymaglyph_artefacts/Release/VST3/GOE Cymaglyph.vst3
# AU: build/GOECymaglyph_artefacts/Release/AU/GOE Cymaglyph.component

# Install (optional)
sudo cp -R "build/GOECymaglyph_artefacts/Release/VST3/GOE Cymaglyph.vst3" /Library/Audio/Plug-Ins/VST3/
sudo cp -R "build/GOECymaglyph_artefacts/Release/AU/GOE Cymaglyph.component" /Library/Audio/Plug-Ins/Components/

# Validate AU (optional)
auval -v aumf GoCy GoEd
```

#### Code Signing (for distribution)
```bash
# Sign the VST3
codesign --force --deep --sign "Developer ID Application: Your Name" \
  "build/GOECymaglyph_artefacts/Release/VST3/GOE Cymaglyph.vst3"

# Sign the AU
codesign --force --deep --sign "Developer ID Application: Your Name" \
  "build/GOECymaglyph_artefacts/Release/AU/GOE Cymaglyph.component"
```

### Windows

```cmd
# Clone the repository
git clone https://github.com/gardenofed/goe-cymaglyph.git
cd goe-cymaglyph

# Configure with Visual Studio
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# The VST3 will be at:
# build\GOECymaglyph_artefacts\Release\VST3\GOE Cymaglyph.vst3

# Install (copy to VST3 folder)
xcopy /E /Y "build\GOECymaglyph_artefacts\Release\VST3\GOE Cymaglyph.vst3" ^
  "%COMMONPROGRAMFILES%\VST3\"
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential cmake git \
  libasound2-dev libjack-jackd2-dev \
  libfreetype6-dev libx11-dev libxinerama-dev \
  libxrandr-dev libxcursor-dev libxcomposite-dev \
  mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev

# Clone and build
git clone https://github.com/gardenofed/goe-cymaglyph.git
cd goe-cymaglyph
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Install
sudo cp -R build/GOECymaglyph_artefacts/Release/VST3/GOE\ Cymaglyph.vst3 \
  /usr/local/lib/vst3/
```

## DAW Setup

### Ableton Live
1. Go to Preferences → Plug-Ins → VST3
2. Add custom VST3 folder if needed
3. Rescan plug-ins
4. Find "GOE Cymaglyph" in Instruments

### Logic Pro
1. The AU version should appear automatically
2. If not, restart Logic Pro
3. Find in Audio Units → Garden of Ed → GOE Cymaglyph

### Reaper
1. Go to Options → Preferences → VST
2. Add VST3 path if needed
3. Rescan
4. Find in FX browser

### FL Studio
1. Options → File Settings
2. Add VST3 folder to search paths
3. Refresh plugin list
4. Find in Plugin Database

## Troubleshooting

### Black/Empty Visualization
- Check OpenGL support: Plugin requires OpenGL 3.3+
- Update graphics drivers
- Try reducing Accuracy parameter
- Disable other GPU-intensive applications

### No Sound
- Check Gate parameter is enabled
- Verify Gain is above 0
- Check DAW track is not muted
- Verify audio interface settings

### High CPU Usage
- Reduce Accuracy parameter
- Lower visual resolution (resize plugin window)
- Disable accumulation (set Grain Amount to 0)

### Plugin Not Found
- Verify installation path matches DAW's plugin folders
- Run validation (auval on macOS)
- Check architecture compatibility (x64 vs ARM)
- Ensure C++ runtime is installed (Windows)

## Performance Notes

- Target: <1% CPU for audio, 60fps visuals on modern hardware
- Accumulation buffer processing is on message thread (non-blocking)
- All audio processing is lock-free and allocation-free
- OpenGL rendering uses simple shaders optimized for real-time

## Development Notes

### Architecture
- **PluginProcessor**: Audio generation, MIDI handling, parameter management
- **PluginEditor**: UI controls and layout
- **Visualizer**: OpenGL rendering and accumulation buffer
- **ModeTables**: Bessel zeros and mode calculations
- **ShaderPrograms**: GLSL shaders for cymatics visualization

### Key Classes and Functions

**PluginProcessor.cpp**
- `processBlock()`: Main audio generation loop
- `handleMidiMessage()`: MIDI note to frequency conversion
- `generateSine/Triangle/Square()`: Waveform generators

**Visualizer.cpp**
- `renderOpenGL()`: Main rendering loop
- `updateModeParameters()`: Frequency to mode mapping
- `updateAccumulationBuffer()`: Grain settling simulation

**ModeTables.h**
- `getSquareModes()`: Square plate mode indices
- `getCircularModes()`: Circular membrane Bessel modes
- `frequencyToModeRank()`: Continuous frequency mapping

### Future Enhancements (TODOs)

#### Metal/Vulkan Backends
- Replace OpenGL with Metal on macOS for better performance
- Add Vulkan support for modern Windows/Linux systems
- Implement compute shaders for accumulation buffer

#### Enhanced Physics
- Implement proper Kirchhoff-Love plate equations
- Add material properties (steel, aluminum, glass)
- Include damping and nonlinear effects
- Real Bessel function evaluation (not approximations)

#### Additional Features
- Multi-voice polyphony with visual superposition
- Record/playback of visual sequences
- Export video of animations
- More color maps and visual styles
- 3D visualization option
- Touch/pressure modulation from MIDI controllers

## License

Copyright © 2024 Garden of Ed. All rights reserved.

## Support

For issues, feature requests, or questions:
- Email: info@gardenofed.com
- GitHub: https://github.com/gardenofed/goe-cymaglyph

---
*GOE Cymaglyph — Cymatics-inspired visual tone generator*