#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setWantsKeyboardFocus (true);
    startTimerHz (30); // drive background animation

    addAndMakeVisible (meterInput);
    addAndMakeVisible (meterOutput);
    meterInput.setColours (juce::Colour::fromRGB (0x27, 0xe8, 0xff),  // glow
                           juce::Colour::fromRGB (0x1f, 0xc7, 0xff),  // fill
                           juce::Colour::fromRGB (0x0a, 0x0c, 0x12)); // back
    meterOutput.setColours (juce::Colour::fromRGB (0xff, 0x2f, 0xd0), // glow
                            juce::Colour::fromRGB (0xf9, 0x62, 0xff), // fill
                            juce::Colour::fromRGB (0x12, 0x0a, 0x14)); // back

    addAndMakeVisible (oscilloscope);

    addAndMakeVisible (parameterGrid);
    parameterGrid.setVisible (false); // hide knobs for now
    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { handleManualLoad(); };

    addAndMakeVisible (bannerButtons);
    bannerButtons.onFullClicked ([this]
    {
        toggleFullScreen();
    });
    bannerButtons.onGearClicked ([this] { showAudioSettings(); });
    bannerButtons.onBypassToggled ([this] (bool enabled) { setBypass (enabled); });
    bannerButtons.onExitClicked ([] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); });

    // Make sure you set the size of the component after
    // you add any child components.
    setSize (1024, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    updateButtonVisibility();
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

bool MainComponent::loadPluginFile (const juce::File& pluginFile, juce::String& errorMessage)
{
    const double sr = currentSampleRate > 0.0 ? currentSampleRate : 44100.0;
    const int bs = currentBlockSize > 0 ? currentBlockSize : 512;

    pluginPrepared = false;
    parameterGrid.setProcessor (nullptr);
    parameterGrid.setVisible (false);

    if (! pluginLoader.load (pluginFile, sr, bs, errorMessage))
        return false;

    if (auto* processor = pluginLoader.get(); processor != nullptr && currentSampleRate > 0.0 && currentBlockSize > 0)
    {
        processor->setPlayConfigDetails (2, 2, currentSampleRate, currentBlockSize);
        processor->prepareToPlay (currentSampleRate, currentBlockSize);
        pluginPrepared = true;
    }

    parameterGrid.setProcessor (pluginLoader.get());
    parameterGrid.setVisible (true);
    showLoadButton = false;
    updateButtonVisibility();

    return true;
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;
    pluginPrepared = false;

    if (! autoLoadAttempted)
    {
        autoLoadAttempted = true;
        juce::File exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
        
        juce::Array<juce::File> candidates;
        candidates.add (exe.getSiblingFile ("Stardust.dll"));
        candidates.add (exe.getParentDirectory().getChildFile ("Resources").getChildFile ("Stardust.dll"));
        candidates.add (juce::File::getCurrentWorkingDirectory().getChildFile ("Stardust.dll"));

        bool loaded = false;
        for (const auto& stardust : candidates)
        {
            if (stardust.existsAsFile())
            {
                juce::String err;
                if (loadPluginFile (stardust, err))
                {
                    loaded = true;
                    break;
                }
            }
        }

        if (! loaded)
        {
            showLoadButton = true;
            updateButtonVisibility();
        }
    }

    if (pluginLoader.isLoaded())
    {
        if (auto* processor = pluginLoader.get())
        {
            processor->setPlayConfigDetails (2, 2, sampleRate, samplesPerBlockExpected);
            processor->prepareToPlay (sampleRate, samplesPerBlockExpected);
            pluginPrepared = true;
        }
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (pluginPrepared && pluginLoader.isLoaded())
    {
        auto* buffer = bufferToFill.buffer;
        const int numSamples = buffer->getNumSamples();
        const int numChannels = buffer->getNumChannels();

        double sumSquaresIn[2] { 0.0, 0.0 };
        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            const float* data = buffer->getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                sumSquaresIn[ch] += data[i] * data[i];
        }
        const double denom = juce::jmax (1, numSamples);
        const float rmsInL = (float) std::sqrt (sumSquaresIn[0] / denom);
        const float rmsInR = (float) std::sqrt (sumSquaresIn[1] / denom);

        float rmsOutL = rmsInL;
        float rmsOutR = rmsInR;

        if (! bypassEnabled)
        {
            juce::MidiBuffer midi;
            pluginLoader.get()->processBlock (*buffer, midi);

            double sumSquaresOut[2] { 0.0, 0.0 };
            for (int ch = 0; ch < numChannels && ch < 2; ++ch)
            {
                const float* data = buffer->getReadPointer (ch);
                for (int i = 0; i < numSamples; ++i)
                    sumSquaresOut[ch] += data[i] * data[i];
            }
            rmsOutL = (float) std::sqrt (sumSquaresOut[0] / denom);
            rmsOutR = (float) std::sqrt (sumSquaresOut[1] / denom);
        }

        rmsInput[0].store (rmsInL);
        rmsInput[1].store (rmsInR);
        rmsOutput[0].store (rmsOutL);
        rmsOutput[1].store (rmsOutR);

        const float maxOut = juce::jmax (rmsOutL, rmsOutR);
        tonalEnergy.store (juce::jlimit (0.0f, 1.0f, maxOut * 2.0f));

        const float envelope = 0.5f * (rmsOutL + rmsOutR);
        const float delta = juce::jmax (0.0f, envelope - prevEnergy);
        prevEnergy = envelope * 0.9f + prevEnergy * 0.1f;
        rhythmEnergy.store (juce::jlimit (0.0f, 1.0f, delta * 8.0f));

        if (buffer->getNumChannels() > 0)
            oscilloscope.pushSamples (buffer->getReadPointer (0), numSamples);
    }
    else
    {
        bufferToFill.clearActiveBufferRegion();
        rmsInput[0].store (0.0f);
        rmsInput[1].store (0.0f);
        rmsOutput[0].store (0.0f);
        rmsOutput[1].store (0.0f);
        tonalEnergy.store (0.0f);
        rhythmEnergy.store (0.0f);
        prevEnergy = 0.0f;
    }
}

void MainComponent::releaseResources()
{
    if (pluginPrepared && pluginLoader.isLoaded())
    {
        if (auto* processor = pluginLoader.get())
            processor->releaseResources();
    }

    pluginPrepared = false;
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Static background neon gradient
    juce::Colour base      = juce::Colour::fromRGB (0x0b, 0x10, 0x18);
    juce::Colour panel     = juce::Colour::fromRGB (0x1a, 0x10, 0x33);
    juce::Colour accentC   = juce::Colour::fromRGB (0x27, 0xe8, 0xff);
    juce::Colour accentM   = juce::Colour::fromRGB (0xff, 0x2f, 0xd0);

    juce::ColourGradient backgroundGrad { panel, bounds.getTopLeft(),
                                          base,  bounds.getBottomRight(), false };
    backgroundGrad.addColour (0.35f, accentM.withAlpha (0.12f));
    backgroundGrad.addColour (0.65f, accentC.withAlpha (0.10f));

    g.setGradientFill (backgroundGrad);
    g.fillRect (bounds);

    // Banner area at top (animated neon hues)
    const float bannerHeight = bounds.getHeight() * 0.08f;
    auto banner = bounds.removeFromTop (bannerHeight);

    const float t = gradientPhase;
    const float tone = juce::jlimit (0.0f, 1.0f, bannerEnergy);
    const float beat = juce::jlimit (0.0f, 1.0f, bannerRhythm);
    const auto accentHueA = 0.76f + 0.14f * tone + 0.06f * std::cos (t * 0.35f);
    const auto accentHueB = accentHueA + 0.08f;

    juce::Colour animAccentM = juce::Colour::fromHSV (accentHueA, 0.80f, 1.00f, 1.0f);
    juce::Colour animAccentC = juce::Colour::fromHSV (accentHueB, 0.75f, 0.95f, 1.0f);

    juce::ColourGradient bannerGrad (animAccentM.withAlpha (0.52f + beat * 0.25f), banner.getX(), banner.getY(),
                                     animAccentC.withAlpha (0.52f + beat * 0.25f), banner.getRight(), banner.getBottom(), false);
    bannerGrad.addColour (0.5f, panel.withAlpha (0.45f));
    g.setGradientFill (bannerGrad);
    g.fillRoundedRectangle (banner.reduced (6.0f), 10.0f);

    // Neon Stardust logo text with glow
    juce::String logoText = "STARDUST";
    static juce::Typeface::Ptr logoFace { juce::Typeface::createSystemTypefaceFor (BinaryData::StarlitNeonRegular_otf,
                                                                                   BinaryData::StarlitNeonRegular_otfSize) };

    juce::Font logoFont;
    if (logoFace != nullptr)
        logoFont = juce::Font (logoFace);
    else
        logoFont = juce::Font ("Hind", banner.getHeight() * 0.6f, juce::Font::bold);

    logoFont.setHeight (banner.getHeight() * 0.6f);
    logoFont.setExtraKerningFactor (0.08f);

    juce::GlyphArrangement arr;
    arr.addLineOfText (logoFont, logoText, banner.getX() + banner.getWidth() * 0.1f, banner.getCentreY() + logoFont.getHeight() * 0.25f);

    juce::Path logoPath;
    arr.createPath (logoPath);

    g.setColour (animAccentM.withAlpha (0.35f));
    g.strokePath (logoPath, juce::PathStrokeType (logoFont.getHeight() * 0.22f));
    g.setColour (animAccentC.withAlpha (0.30f));
    g.strokePath (logoPath, juce::PathStrokeType (logoFont.getHeight() * 0.16f));

    juce::ColourGradient logoGrad (animAccentM, banner.getX(), banner.getY(),
                                   animAccentC, banner.getRight(), banner.getBottom(), false);
    logoGrad.addColour (0.5f, animAccentM.brighter (0.3f));
    g.setGradientFill (logoGrad);
    g.fillPath (logoPath);

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (logoPath, juce::PathStrokeType (1.8f));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    // Reserve top banner space
    const int bannerHeight = (int) (area.getHeight() * 0.08f);
    auto bannerArea = area.removeFromTop (bannerHeight);

    const int buttonSize = juce::jmax (24, (int) (bannerHeight * 0.6f));
    bannerButtons.setBounds (bannerArea.removeFromRight (buttonSize * 4 + 32));
    bannerButtons.setButtonSize (buttonSize);

    auto content = area;

    const int meterWidth = juce::jmax (16, area.getWidth() / 30);
    meterInput.setBounds (content.removeFromLeft (meterWidth).reduced (4, 6));
    meterOutput.setBounds (content.removeFromRight (meterWidth).reduced (4, 6));

    const int oscHeight = 140;
    auto oscArea = content.removeFromBottom (oscHeight).reduced (8, 6);
    oscilloscope.setBounds (oscArea);

    parameterGrid.setBounds (content.reduced (8));

    const auto buttonSize2 = (int) (juce::jmin (area.getWidth(), area.getHeight()) * 0.1f);
    auto buttonBounds = juce::Rectangle<int> (buttonSize2, buttonSize2)
                            .withCentre (area.getCentre());
    loadButton.setBounds (buttonBounds);
}

void MainComponent::updateButtonVisibility()
{
    loadButton.setVisible (showLoadButton);
    loadButton.toFront (false);
}

void MainComponent::handleManualLoad()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Select Stardust.dll", juce::File(), "*.dll");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooserRef)
    {
        auto chosen = chooserRef.getResult();
        juce::String err;

        if (chosen.existsAsFile() && loadPluginFile (chosen, err))
        {
            showLoadButton = false;
        }
        else
        {
            showLoadButton = true;
            if (err.isEmpty())
                err = "Failed to load plugin.";
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::WarningIcon, "Load failed", err);
        }

        updateButtonVisibility();
        fileChooser.reset();
    });
}

void MainComponent::showAudioSettings()
{
    auto component = std::make_unique<juce::AudioDeviceSelectorComponent> (deviceManager,
                                                                           0, 2,  // min/max inputs
                                                                           0, 2,  // min/max outputs
                                                                           true, true, true, false);
    component->setSize (500, 400);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (component.release());
    options.dialogTitle = "Audio Settings";
    
    // Customization: Set the background colour to match your MainComponent's gradient start colour
    options.dialogBackgroundColour = juce::Colour::fromRGB (11, 16, 24);
    
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    options.launchAsync();
}

void MainComponent::timerCallback()
{
    gradientPhase += 0.7f; // advance animation
    if (gradientPhase > juce::MathConstants<float>::twoPi)
        gradientPhase -= juce::MathConstants<float>::twoPi;
    
    meterInput.setTargetLevels (rmsInput[0].load(), rmsInput[1].load());
    meterOutput.setTargetLevels (rmsOutput[0].load(), rmsOutput[1].load());
    meterInput.tick (0.18f);
    meterOutput.tick (0.18f);
    bannerEnergy = bannerEnergy * 0.9f + tonalEnergy.load() * 0.1f;
    bannerRhythm = bannerRhythm * 0.85f + rhythmEnergy.load() * 0.15f;
    oscilloscope.repaint();
    repaint();
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::F11Key)
    {
        toggleFullScreen();
        return true;
    }

    return false;
}

void MainComponent::toggleFullScreen()
{
    if (auto* tlc = getTopLevelComponent())
    {
        if (auto* dw = dynamic_cast<juce::DocumentWindow*> (tlc))
            dw->setFullScreen (! dw->isFullScreen());
    }
}

void MainComponent::setBypass (bool shouldBypass)
{
    bypassEnabled = shouldBypass;
}
