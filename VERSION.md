# Version History

## v0.1.0 (2024-08-27)
### Initial Release
- ✨ Complete VST3/AU plugin implementation
- 🎵 Tone generation with sine, triangle, and square waves
- 🌊 Real-time cymatics visualization with OpenGL
- 🎨 Three visualization mediums: Plate, Membrane, Water
- 🎹 Full MIDI support with adjustable A4 reference
- 🔄 Frequency sweep with configurable rate and range
- 💾 8 factory presets showcasing different modes
- 🎯 Accumulation buffer for grain/sand effects
- 🎛️ Full parameter automation support
- 📸 PNG export of visualizations

### Technical Details
- C++20 with JUCE 7.0.12
- OpenGL 3.3+ for rendering
- Lock-free audio processing
- Thread-safe visualization updates
- Cross-platform: macOS (Universal Binary), Windows, Linux

### Known Issues
- Build process requires downloading JUCE (may take several minutes)
- OpenGL context creation may fail on some older GPUs
- Accumulation buffer reset needed when changing major parameters

### Contributors
- Eden Grill
- Claude (AI Assistant)

---

## Planned Features (v0.2.0)
- [ ] Metal rendering backend for macOS
- [ ] Improved Bessel function accuracy
- [ ] Additional color maps
- [ ] Video export capability
- [ ] More presets
- [ ] Performance optimizations