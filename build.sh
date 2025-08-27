#!/bin/bash
set -e

echo "Building GOE Cymaglyph..."

# Clean previous build
rm -rf build

# Configure
echo "Configuring..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

# Build
echo "Building..."
cmake --build build --config Release -j8

echo "Build complete!"
echo "VST3 plugin location: build/GOECymaglyph_artefacts/Release/VST3/GOE Cymaglyph.vst3"
echo "AU plugin location: build/GOECymaglyph_artefacts/Release/AU/GOE Cymaglyph.component"