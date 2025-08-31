cmake_minimum_required(VERSION 3.22)
project(SandWizard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

# Add the plugin target
juce_add_plugin(SandWizard
    COMPANY_NAME "Garden of Ed"
    COMPANY_WEBSITE "https://gardenofed.com"
    COMPANY_EMAIL "info@gardenofed.com"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES "Instrument" "Fx"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
    FORMATS VST3 AU AUv3
    PRODUCT_NAME "Sand Wizard"
    PLUGIN_MANUFACTURER_CODE "GoEd"
    PLUGIN_CODE "SaWz"
    BUNDLE_ID "com.gardenofed.sandwizard"
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE
    REQUIRES_FULL_SCREEN FALSE
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
)

# Add source files
target_sources(SandWizard PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/Visualizer.h
    Source/Visualizer.cpp
    Source/ModeTables.h
    Source/ShaderPrograms.h
    Source/SynthEngine.h
    Source/SynthEngine.cpp
    Source/EnhancedVisualizer.h
    Source/EnhancedVisualizer.cpp
    Source/SettingsPanel.h
    Source/SettingsPanel.cpp
    Source/ControlPanel.h
    Source/ControlPanel.cpp
)

# Add JUCE module paths
target_include_directories(SandWizard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Platform-specific settings
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13")
endif()

# Compile definitions
target_compile_definitions(SandWizard
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_MODAL_LOOPS_PERMITTED=0
)

# Link libraries
target_link_libraries(SandWizard
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Set warning levels
if(MSVC)
    target_compile_options(SandWizard PRIVATE /W4)
else()
    target_compile_options(SandWizard PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy presets
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Presets")
    add_custom_command(TARGET SandWizard POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Presets"
        "$<TARGET_FILE_DIR:SandWizard>/Presets"
    )
endif()