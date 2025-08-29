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
    
    // Solid dark background for better visibility
    g.setColour(juce::Colours::black.withAlpha(0.95f * opacity));
    g.fillRect(bounds);
    
    // Add spacing from top
    bounds.removeFromTop(100);
    
    // Title positioned lower and centered
    g.setColour(juce::Colours::white.withAlpha(opacity));
    g.setFont(juce::Font("Arial", 28.0f, juce::Font::bold));
    g.drawText("SAND WIZARD by Garden of Eden", bounds.removeFromTop(50), juce::Justification::centred);
    
    // Add spacing before mode cards
    bounds.removeFromTop(40);
    
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
            // Selected mode highlight with sharp corners
            g.setColour(juce::Colours::white.withAlpha(0.2f * cardOpacity));
            g.fillRect(card.bounds.expanded(4));
        }
        
        // Solid color background with sharp corners
        g.setColour(card.isHovered ? 
                   modeInfo.primaryColor.withAlpha(0.8f * cardOpacity) :
                   modeInfo.primaryColor.withAlpha(0.5f * cardOpacity));
        g.fillRect(card.bounds);
        
        // Thick white border with sharp corners
        g.setColour(card.isHovered ? 
                   juce::Colours::white.withAlpha(cardOpacity) :
                   juce::Colours::white.withAlpha(0.7f * cardOpacity));
        g.drawRect(card.bounds, card.isHovered ? 3.0f : 2.0f);
        
        // Mode name with smaller minimal font
        g.setColour(juce::Colours::white.withAlpha(cardOpacity));
        g.setFont(juce::Font("Arial", 16.0f, juce::Font::bold));
        auto nameArea = card.bounds;
        g.drawText(modeInfo.name, nameArea.removeFromTop(45), juce::Justification::centred);
        
        // Mode description with minimal font
        g.setFont(juce::Font("Arial", 11.0f, juce::Font::plain));
        g.setColour(juce::Colours::white.withAlpha(0.9f * cardOpacity));
        auto descArea = card.bounds;
        descArea.removeFromTop(45);
        g.drawText(modeInfo.description, descArea, juce::Justification::centred);
    }
    
    // Mono/Poly toggle with sharp corners
    auto toggleBounds = monoPolyToggle;
    g.setColour(monoPolyHovered ? 
               juce::Colours::white.withAlpha(0.9f * opacity) :
               juce::Colours::white.withAlpha(0.6f * opacity));
    g.drawRect(toggleBounds, 2.0f);
    
    // Toggle background with sharp corners
    g.setColour(monoPolyHovered ?
               juce::Colours::white.withAlpha(0.3f * opacity) :
               juce::Colours::white.withAlpha(0.2f * opacity));
    g.fillRect(toggleBounds);
    
    // Toggle text with minimal font
    g.setFont(juce::Font("Arial", 18.0f, juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(opacity));
    juce::String modeText = monoMode ? "MONOPHONIC" : "POLYPHONIC";
    g.drawText(modeText, toggleBounds, juce::Justification::centred);
    
    // Footer hint with minimal text
    g.setFont(juce::Font("Arial", 12.0f, juce::Font::plain));
    g.setColour(juce::Colours::white.withAlpha(0.8f * opacity));
    g.drawText("Settings appear after 0.5 seconds of silence", 
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
    bounds.removeFromTop(190); // Space for repositioned title
    bounds.removeFromBottom(100); // Space for mono/poly toggle and footer
    
    // Layout mode cards in a grid
    const int cardsPerRow = 5;
    const int numRows = 2;
    float cardWidth = 160.0f;
    float cardHeight = 140.0f;
    float spacing = 20.0f;
    
    float totalWidth = cardsPerRow * cardWidth + (cardsPerRow - 1) * spacing;
    float totalHeight = numRows * cardHeight + (numRows - 1) * spacing;
    
    // Make sure cards fit in window
    if (totalWidth > bounds.getWidth())
    {
        float scale = (bounds.getWidth() - 40) / totalWidth;
        cardWidth *= scale;
        cardHeight *= scale;
        spacing *= scale;
        totalWidth = cardsPerRow * cardWidth + (cardsPerRow - 1) * spacing;
        totalHeight = numRows * cardHeight + (numRows - 1) * spacing;
    }
    
    float startX = (getWidth() - totalWidth) * 0.5f;
    float startY = bounds.getY() + (bounds.getHeight() - totalHeight) * 0.5f;
    
    for (int i = 0; i < SynthEngine::NumModes; i++)
    {
        int row = i / cardsPerRow;
        int col = i % cardsPerRow;
        
        float x = startX + col * (cardWidth + spacing);
        float y = startY + row * (cardHeight + spacing);
        
        modeCards[i].bounds = juce::Rectangle<float>(x, y, cardWidth, cardHeight);
    }
    
    // Position mono/poly toggle - larger for better clicking
    float toggleY = startY + totalHeight + 40.0f;
    monoPolyToggle = juce::Rectangle<float>(
        (getWidth() - 250.0f) * 0.5f, 
        toggleY, 
        250.0f, 
        50.0f
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