#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SynthEngine.h"
#include <functional>

class SettingsPanel : public juce::Component
{
public:
    SettingsPanel();
    ~SettingsPanel() override;
    
    // Component overrides
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    
    // Visibility control
    void setVisible(bool shouldBeVisible, bool animate = true);
    bool isFullyVisible() const { return opacity >= 0.95f; }
    
    // Get current settings
    int getSelectedMode() const { return selectedMode; }
    bool isMonophonic() const { return monoMode; }
    int getOctaveShift() const { return octaveShift; }
    
    // Callbacks
    std::function<void(int)> onModeSelected;
    std::function<void(bool)> onMonoPolyChanged;
    std::function<void(int)> onOctaveChanged;
    
private:
    // Mode card for visual selection
    struct ModeCard
    {
        juce::Rectangle<float> bounds;
        int modeIndex;
        bool isHovered = false;
        float hoverAnimation = 0.0f;
        
        void paint(juce::Graphics& g, const SynthEngine::ModeInfo& info);
    };
    
    // UI Components
    std::array<ModeCard, SynthEngine::NumModes> modeCards;
    juce::Rectangle<float> monoPolyToggle;
    bool monoPolyHovered = false;
    
    // Octave control
    juce::Rectangle<float> octaveDownButton;
    juce::Rectangle<float> octaveUpButton;
    juce::Rectangle<float> octaveDisplay;
    bool octaveDownHovered = false;
    bool octaveUpHovered = false;
    
    // Current state
    int selectedMode = 0;
    bool monoMode = true;
    int octaveShift = 0; // Range: -2 to +2
    
    // Animation
    float opacity = 0.0f;
    float targetOpacity = 0.0f;
    
    // Layout helpers
    void layoutModeCards();
    int getModeCardAt(juce::Point<int> point);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPanel)
};