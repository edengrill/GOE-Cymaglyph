#include "SettingsPanel.h"

SettingsPanel::SettingsPanel()
{
    setOpaque(false);
    setInterceptsMouseClicks(true, true);
    
    // Initialize mode cards
    for (int i = 0; i < SynthEngine::NumModes; i++)
    {
        modeCards[i].modeIndex = i;
    }
}

SettingsPanel::~SettingsPanel()
{
}

void SettingsPanel::paint(juce::Graphics& g)
{
    if (opacity < 0.01f) return;
    
    auto bounds = getLocalBounds();
    
    // Semi-transparent background
    g.setColour(juce::Colours::black.withAlpha(0.8f * opacity));
    g.fillRect(bounds);
    
    // Title
    g.setColour(juce::Colours::white.withAlpha(opacity));
    g.setFont(28.0f);
    g.drawText("GOE CYMAGLYPH v3.0", bounds.removeFromTop(60), juce::Justification::centred);
    
    // Instructions
    g.setFont(16.0f);
    g.setColour(juce::Colours::white.withAlpha(0.7f * opacity));
    juce::String instructions = "Click a mode to select • Play notes on your MIDI keyboard";
    g.drawText(instructions, bounds.removeFromTop(30), juce::Justification::centred);
    
    // Draw mode cards
    for (int i = 0; i < SynthEngine::NumModes; i++)
    {
        auto& card = modeCards[i];
        auto modeInfo = SynthEngine::getModeInfo(i);
        
        // Card background
        float cardOpacity = opacity;
        if (card.isHovered)
        {
            cardOpacity *= (1.0f + card.hoverAnimation * 0.3f);
        }
        
        if (i == selectedMode)
        {
            // Selected mode highlight
            g.setColour(juce::Colours::white.withAlpha(0.2f * cardOpacity));
            g.fillRoundedRectangle(card.bounds.expanded(4), 12.0f);
        }
        
        // Card gradient background
        juce::ColourGradient gradient(
            modeInfo.primaryColor.withAlpha(0.6f * cardOpacity),
            card.bounds.getTopLeft(),
            modeInfo.secondaryColor.withAlpha(0.4f * cardOpacity),
            card.bounds.getBottomRight(),
            false
        );
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(card.bounds, 10.0f);
        
        // Card border
        g.setColour(card.isHovered ? 
                   juce::Colours::white.withAlpha(0.8f * cardOpacity) :
                   juce::Colours::white.withAlpha(0.3f * cardOpacity));
        g.drawRoundedRectangle(card.bounds, 10.0f, 2.0f);
        
        // Mode name
        g.setColour(juce::Colours::white.withAlpha(cardOpacity));
        g.setFont(18.0f);
        g.drawText(modeInfo.name, card.bounds.removeFromTop(40), juce::Justification::centred);
        
        // Mode description
        g.setFont(12.0f);
        g.setColour(juce::Colours::white.withAlpha(0.7f * cardOpacity));
        g.drawText(modeInfo.description, card.bounds, juce::Justification::centred);
    }
    
    // Mono/Poly toggle
    auto toggleBounds = monoPolyToggle;
    g.setColour(monoPolyHovered ? 
               juce::Colours::white.withAlpha(0.9f * opacity) :
               juce::Colours::white.withAlpha(0.6f * opacity));
    g.drawRoundedRectangle(toggleBounds, 8.0f, 2.0f);
    
    // Toggle background
    g.setColour(juce::Colours::white.withAlpha(0.1f * opacity));
    g.fillRoundedRectangle(toggleBounds, 8.0f);
    
    // Toggle text
    g.setFont(20.0f);
    g.setColour(juce::Colours::white.withAlpha(opacity));
    juce::String modeText = monoMode ? "MONOPHONIC" : "POLYPHONIC";
    g.drawText(modeText, toggleBounds, juce::Justification::centred);
    
    // Footer hint
    g.setFont(14.0f);
    g.setColour(juce::Colours::white.withAlpha(0.5f * opacity));
    g.drawText("Settings appear after 2 seconds of silence", 
               getLocalBounds().removeFromBottom(30), juce::Justification::centred);
}

void SettingsPanel::resized()
{
    layoutModeCards();
}

void SettingsPanel::mouseDown(const juce::MouseEvent& event)
{
    auto point = event.getPosition();
    
    // Check mode cards
    int clickedMode = getModeCardAt(point);
    if (clickedMode >= 0)
    {
        selectedMode = clickedMode;
        if (onModeSelected)
            onModeSelected(selectedMode);
        repaint();
        return;
    }
    
    // Check mono/poly toggle
    if (monoPolyToggle.contains(point.toFloat()))
    {
        monoMode = !monoMode;
        if (onMonoPolyChanged)
            onMonoPolyChanged(monoMode);
        repaint();
    }
}

void SettingsPanel::mouseMove(const juce::MouseEvent& event)
{
    auto point = event.getPosition();
    
    // Update hover states for mode cards
    for (auto& card : modeCards)
    {
        bool wasHovered = card.isHovered;
        card.isHovered = card.bounds.contains(point.toFloat());
        
        if (card.isHovered != wasHovered)
        {
            card.hoverAnimation = card.isHovered ? 1.0f : 0.0f;
            repaint();
        }
    }
    
    // Update mono/poly hover
    bool wasMonoPolyHovered = monoPolyHovered;
    monoPolyHovered = monoPolyToggle.contains(point.toFloat());
    if (monoPolyHovered != wasMonoPolyHovered)
    {
        repaint();
    }
}

void SettingsPanel::setVisible(bool shouldBeVisible, bool animate)
{
    targetOpacity = shouldBeVisible ? 1.0f : 0.0f;
    
    if (!animate)
    {
        opacity = targetOpacity;
        repaint();
    }
    else
    {
        // Animate opacity change
        juce::Timer::callAfterDelay(16, [this]() {
            if (std::abs(opacity - targetOpacity) > 0.01f)
            {
                opacity += (targetOpacity - opacity) * 0.15f;
                repaint();
                setVisible(targetOpacity > 0.5f, true);
            }
        });
    }
}

void SettingsPanel::layoutModeCards()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(120); // Space for title and instructions
    bounds.removeFromBottom(80); // Space for mono/poly toggle and footer
    
    // Layout mode cards in a grid
    const int cardsPerRow = 5;
    const int numRows = 2;
    const float cardWidth = 140.0f;
    const float cardHeight = 100.0f;
    const float spacing = 20.0f;
    
    float totalWidth = cardsPerRow * cardWidth + (cardsPerRow - 1) * spacing;
    float totalHeight = numRows * cardHeight + (numRows - 1) * spacing;
    
    float startX = (bounds.getWidth() - totalWidth) * 0.5f;
    float startY = bounds.getY() + (bounds.getHeight() - totalHeight) * 0.5f;
    
    for (int i = 0; i < SynthEngine::NumModes; i++)
    {
        int row = i / cardsPerRow;
        int col = i % cardsPerRow;
        
        float x = startX + col * (cardWidth + spacing);
        float y = startY + row * (cardHeight + spacing);
        
        modeCards[i].bounds = juce::Rectangle<float>(x, y, cardWidth, cardHeight);
    }
    
    // Position mono/poly toggle
    float toggleY = startY + totalHeight + 40.0f;
    monoPolyToggle = juce::Rectangle<float>(
        (getWidth() - 200.0f) * 0.5f, 
        toggleY, 
        200.0f, 
        40.0f
    );
}

int SettingsPanel::getModeCardAt(juce::Point<int> point)
{
    for (int i = 0; i < SynthEngine::NumModes; i++)
    {
        if (modeCards[i].bounds.contains(point.toFloat()))
        {
            return i;
        }
    }
    return -1;
}