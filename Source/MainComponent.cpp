#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    addAndMakeVisible (parameterGrid);
    parameterGrid.setVisible (false); // hide knobs for now
    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { handleManualLoad(); };

    addAndMakeVisible (audioSettingsButton);
    audioSettingsButton.setButtonText ("\u2699");
    audioSettingsButton.onClick = [this] { showAudioSettings(); };

    // Load background image from Resources directory next to the executable
    {
        juce::File exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
        juce::File imgFile = exe.getParentDirectory().getChildFile ("Resources").getChildFile ("StardustBackground.png");
        if (imgFile.existsAsFile())
            backgroundImage = juce::ImageFileFormat::loadFrom (imgFile);
    }

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
        juce::MidiBuffer midi;
        pluginLoader.get()->processBlock (*bufferToFill.buffer, midi);
    }
    else
    {
        bufferToFill.clearActiveBufferRegion();
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

    if (backgroundImage.isValid())
    {
        g.drawImage (backgroundImage, bounds, juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        juce::ColourGradient gradient { juce::Colour::fromRGB (11, 16, 24), bounds.getTopLeft(),
                                        juce::Colour::fromRGB (9, 28, 36), bounds.getBottomRight(), false };
        gradient.addColour (0.5f, juce::Colour::fromRGB (14, 38, 51));

        g.setGradientFill (gradient);
        g.fillRect (bounds);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    parameterGrid.setBounds (area.reduced (8));

    const auto buttonSize = (int) (juce::jmin (area.getWidth(), area.getHeight()) * 0.1f);
    auto buttonBounds = juce::Rectangle<int> (buttonSize, buttonSize)
                            .withCentre (area.getCentre());
    loadButton.setBounds (buttonBounds);

    const int settingsSize = 32;
    auto settingsBounds = juce::Rectangle<int> (settingsSize, settingsSize);
    settingsBounds.setPosition (area.getBottomRight() - juce::Point<int> (settingsSize, settingsSize) - juce::Point<int> (8, 8));
    audioSettingsButton.setBounds (settingsBounds);
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
