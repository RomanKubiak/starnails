#pragma once

#include <JuceHeader.h>

class IconButton : public juce::Button
{
public:
    enum class IconType { Fullscreen, Gear, Exit, Bypass };

    explicit IconButton (IconType t) : juce::Button ("icon"), type (t)
    {
        if (type == IconType::Bypass)
            setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (4.0f);
        const auto baseColour = juce::Colour::fromRGB (0x27, 0xe8, 0xff);
        const auto accent     = juce::Colour::fromRGB (0xff, 0x2f, 0xd0);
        auto colour = baseColour; 
        if (shouldDrawButtonAsDown)      colour = accent;
        else if (shouldDrawButtonAsHighlighted) colour = baseColour.brighter (0.3f);
        if (getToggleState() && type == IconType::Bypass)
            colour = accent.brighter (0.2f);

        g.setColour (juce::Colour::fromRGB (0x0b, 0x10, 0x18).withAlpha (0.8f));
        g.fillRoundedRectangle (bounds.expanded (2.0f), 6.0f);
        g.setColour (colour.withAlpha (0.6f));
        g.drawRoundedRectangle (bounds.expanded (2.0f), 6.0f, 1.6f);

        g.setColour (colour);

        switch (type)
        {
            case IconType::Exit:
            {
                juce::Path p;
                p.startNewSubPath (bounds.getX(), bounds.getY());
                p.lineTo (bounds.getRight(), bounds.getBottom());
                p.startNewSubPath (bounds.getRight(), bounds.getY());
                p.lineTo (bounds.getX(), bounds.getBottom());
                g.strokePath (p, juce::PathStrokeType (2.5f));
                break;
            }
            case IconType::Fullscreen:
            {
                const float pad = bounds.getWidth() * 0.18f;
                juce::Path p;
                // top-left
                p.startNewSubPath (bounds.getX() + pad, bounds.getY());
                p.lineTo (bounds.getX(), bounds.getY());
                p.lineTo (bounds.getX(), bounds.getY() + pad);
                // top-right
                p.startNewSubPath (bounds.getRight() - pad, bounds.getY());
                p.lineTo (bounds.getRight(), bounds.getY());
                p.lineTo (bounds.getRight(), bounds.getY() + pad);
                // bottom-left
                p.startNewSubPath (bounds.getX(), bounds.getBottom() - pad);
                p.lineTo (bounds.getX(), bounds.getBottom());
                p.lineTo (bounds.getX() + pad, bounds.getBottom());
                // bottom-right
                p.startNewSubPath (bounds.getRight(), bounds.getBottom() - pad);
                p.lineTo (bounds.getRight(), bounds.getBottom());
                p.lineTo (bounds.getRight() - pad, bounds.getBottom());
                g.strokePath (p, juce::PathStrokeType (2.5f));
                break;
            }
            case IconType::Gear:
            {
                auto centre = bounds.getCentre();
                const float rOuter = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.42f;
                const float rInner = rOuter * 0.55f;
                juce::Path p;
                const int teeth = 8;
                for (int i = 0; i < teeth; ++i)
                {
                    const float angle = juce::MathConstants<float>::twoPi * i / (float) teeth;
                    const float a1 = angle - 0.08f;
                    const float a2 = angle + 0.08f;
                    juce::Point<float> p1 (centre.x + rInner * std::cos (a1), centre.y + rInner * std::sin (a1));
                    juce::Point<float> p2 (centre.x + rOuter * std::cos (angle), centre.y + rOuter * std::sin (angle));
                    juce::Point<float> p3 (centre.x + rInner * std::cos (a2), centre.y + rInner * std::sin (a2));
                    if (i == 0) p.startNewSubPath (p1);
                    p.lineTo (p2);
                    p.lineTo (p3);
                }
                p.closeSubPath();
                g.fillPath (p);
                g.setColour (juce::Colour::fromRGB (0x0b, 0x10, 0x18));
                g.fillEllipse (bounds.reduced (bounds.getWidth() * 0.35f));
                g.setColour (colour);
                g.drawEllipse (bounds.reduced (bounds.getWidth() * 0.35f), 1.4f);
                break;
            }
            case IconType::Bypass:
            {
                // Two wires with optional bridge
                const bool connected = getToggleState();
                const float y = bounds.getCentreY();
                const float xLeft = bounds.getX() + bounds.getWidth() * 0.12f;
                const float xRight = bounds.getRight() - bounds.getWidth() * 0.12f;
                const float gap = bounds.getWidth() * 0.12f;
                const float bridge = connected ? gap : gap * 1.4f;

                const float midLeft = bounds.getCentreX() - bridge * 0.5f;
                const float midRight = bounds.getCentreX() + bridge * 0.5f;

                juce::Path wireL; wireL.startNewSubPath (xLeft, y); wireL.lineTo (midLeft, y);
                juce::Path wireR; wireR.startNewSubPath (midRight, y); wireR.lineTo (xRight, y);
                g.strokePath (wireL, juce::PathStrokeType (2.6f));
                g.strokePath (wireR, juce::PathStrokeType (2.6f));

                if (connected)
                {
                    juce::Path p;
                    p.startNewSubPath (midLeft, y);
                    p.cubicTo (midLeft + bridge * 0.15f, y - bridge * 0.25f,
                               midRight - bridge * 0.15f, y + bridge * 0.25f,
                               midRight, y);
                    g.strokePath (p, juce::PathStrokeType (2.6f));
                }
                else
                {
                    const float r = 3.0f;
                    g.fillEllipse (midLeft - r, y - r, r * 2.0f, r * 2.0f);
                    g.fillEllipse (midRight - r, y - r, r * 2.0f, r * 2.0f);
                }
                break;
            }
        }
    }

private:
    IconType type;
};

class BannerButtons : public juce::Component
{
public:
    BannerButtons();

    void resized() override;

    void onExitClicked (std::function<void()> cb)   { exitCallback = std::move (cb); }
    void onGearClicked (std::function<void()> cb)   { gearCallback = std::move (cb); }
    void onFullClicked (std::function<void()> cb)   { fullCallback = std::move (cb); }
    void onBypassToggled (std::function<void(bool)> cb) { bypassCallback = std::move (cb); }

    void setButtonSize (int newSize) { buttonSize = newSize; resized(); }

private:
    IconButton fullScreenButton { IconButton::IconType::Fullscreen };
    IconButton audioSettingsButton { IconButton::IconType::Gear };
    IconButton bypassButton { IconButton::IconType::Bypass };
    IconButton exitButton { IconButton::IconType::Exit };

    int buttonSize { 28 };

    std::function<void()> exitCallback;
    std::function<void()> gearCallback;
    std::function<void()> fullCallback;
    std::function<void(bool)> bypassCallback;
};
