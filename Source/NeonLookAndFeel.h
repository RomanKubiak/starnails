#pragma once

#include <JuceHeader.h>

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel()
    {
        background    = juce::Colour::fromRGB (0x0b, 0x10, 0x18); // deep night
        panel         = juce::Colour::fromRGB (0x1a, 0x10, 0x33); // indigo
        accentMagenta = juce::Colour::fromRGB (0xff, 0x2f, 0xd0); // neon magenta
        accentCyan    = juce::Colour::fromRGB (0x27, 0xe8, 0xff); // neon cyan
        textMain      = juce::Colour::fromRGB (0xf4, 0xf8, 0xff); // near-white

        setDefaultSansSerifTypefaceName ("Hind");

        setColour (juce::ResizableWindow::backgroundColourId, background);
        setColour (juce::PopupMenu::backgroundColourId, background);
        setColour (juce::PopupMenu::textColourId, textMain);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, panel.brighter (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId, accentCyan);

        setColour (juce::TextButton::buttonColourId, panel);
        setColour (juce::TextButton::buttonOnColourId, accentMagenta);
        setColour (juce::TextButton::textColourOnId, textMain);
        setColour (juce::TextButton::textColourOffId, textMain);

        setColour (juce::ComboBox::backgroundColourId, panel);
        setColour (juce::ComboBox::outlineColourId, accentMagenta.withAlpha (0.7f));
        setColour (juce::ComboBox::textColourId, textMain);

        setColour (juce::Label::textColourId, textMain);

        setColour (juce::Slider::trackColourId, accentMagenta);
        setColour (juce::Slider::backgroundColourId, panel);
        setColour (juce::Slider::thumbColourId, accentCyan);

        setColour (juce::ScrollBar::thumbColourId, accentMagenta.withAlpha (0.6f));
        setColour (juce::TooltipWindow::textColourId, textMain);
        setColour (juce::TooltipWindow::backgroundColourId, background);
        setColour (juce::TooltipWindow::outlineColourId, accentMagenta);
    }

    ~NeonLookAndFeel() override = default;

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        auto f = font;
        f.setTypefaceName ("Hind");
        f.setTypefaceStyle ("Regular");
        return juce::LookAndFeel_V4::getTypefaceForFont (f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto base = backgroundColour.darker (0.3f);
        auto accent = isButtonDown ? accentMagenta : accentCyan;

        g.setColour (base);
        g.fillRoundedRectangle (bounds.reduced (1.0f), 6.0f);

        juce::ColourGradient grad (accent, bounds.getCentreX(), bounds.getY(),
                                   accentMagenta, bounds.getCentreX(), bounds.getBottom(), false);
        grad.addColour (0.5, accentCyan);

        g.setGradientFill (grad);
        g.drawRoundedRectangle (bounds.reduced (1.5f), 6.0f, isMouseOverButton ? 2.5f : 1.5f);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox& box) override
    {
        juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH, box);

        auto bounds = juce::Rectangle<float> (1.0f, 1.0f, (float) width - 2.0f, (float) height - 2.0f);
        g.setColour (panel);
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (accentMagenta.withAlpha (0.8f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.5f);

        auto arrowZone = bounds.removeFromRight (bounds.getHeight()).reduced (4.0f);
        juce::Path p;
        p.addTriangle (arrowZone.getX(), arrowZone.getY() + 4.0f,
                       arrowZone.getRight(), arrowZone.getY() + 4.0f,
                       arrowZone.getCentreX(), arrowZone.getBottom() - 4.0f);
        g.setColour (isButtonDown ? accentCyan : textMain);
        g.fillPath (p);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        juce::ignoreUnused (minSliderPos, maxSliderPos, style, slider);
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);

        auto track = bounds.reduced (0.0f, bounds.getHeight() * 0.18f); // thicker track
        auto radius = track.getHeight() * 0.4f; // more oval ends
        g.setColour (panel);
        g.fillRoundedRectangle (track, radius);

        auto fill = track.withRight (sliderPos);
        juce::ColourGradient grad (accentMagenta, fill.getX(), fill.getCentreY(),
                                   accentCyan, fill.getRight(), fill.getCentreY(), false);
        grad.addColour (0.5, accentMagenta.brighter (0.2f));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, radius);

        auto thumbSize = track.getHeight() * 1.2f;
        auto thumbX = juce::jlimit (track.getX(), track.getRight(), sliderPos);
        auto thumbRect = juce::Rectangle<float> (thumbSize, thumbSize).withCentre ({ thumbX, track.getCentreY() });
        g.setColour (accentCyan);
        g.fillEllipse (thumbRect);
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawEllipse (thumbRect, 1.0f);
    }

private:
    juce::Colour background;    // deep base
    juce::Colour panel;         // panel / track base
    juce::Colour accentMagenta; // primary accent
    juce::Colour accentCyan;    // secondary accent
    juce::Colour textMain;
};
