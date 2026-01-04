#include "BannerButtons.h"

BannerButtons::BannerButtons()
{
    addAndMakeVisible (exitButton);
    addAndMakeVisible (audioSettingsButton);
    addAndMakeVisible (fullScreenButton);
    addAndMakeVisible (bypassButton);

    exitButton.onClick = [this] { if (exitCallback) exitCallback(); };
    audioSettingsButton.onClick = [this] { if (gearCallback) gearCallback(); };
    fullScreenButton.onClick = [this] { if (fullCallback) fullCallback(); };
    bypassButton.onClick = [this] { if (bypassCallback) bypassCallback (bypassButton.getToggleState()); };
}

void BannerButtons::resized()
{
    auto area = getLocalBounds();
    const int margin = 6;
    const int h = area.getHeight();
    const int size = juce::jlimit (20, h, buttonSize);

    auto right = area.removeFromRight (size * 4 + margin * 3);
    auto exitBounds = right.removeFromRight (size + margin);
    auto bypassBounds = right.removeFromRight (size + margin);
    auto gearBounds = right.removeFromRight (size + margin);
    auto fullBounds = right.removeFromRight (size + margin);

    exitButton.setBounds (exitBounds.withSizeKeepingCentre (size, size));
    bypassButton.setBounds (bypassBounds.withSizeKeepingCentre (size, size));
    audioSettingsButton.setBounds (gearBounds.withSizeKeepingCentre (size, size));
    fullScreenButton.setBounds (fullBounds.withSizeKeepingCentre (size, size));
}
