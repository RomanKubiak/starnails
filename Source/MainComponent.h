#pragma once

#include <JuceHeader.h>
#include "SinglePluginLoader.h"
#include "StardustEditor.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    bool loadPluginFile (const juce::File& pluginFile, juce::String& errorMessage);

private:
    //==============================================================================
    // Your private member variables go here...

    StardustEditor parameterGrid;
    juce::TextButton loadButton { "Load Stardust" };
    juce::TextButton audioSettingsButton { "" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Image backgroundImage;
    SinglePluginLoader pluginLoader;
    bool pluginPrepared = false;
    double currentSampleRate = 0.0;
    int currentBlockSize = 0;
    bool autoLoadAttempted = false;
    bool showLoadButton = false;

    void updateButtonVisibility();
    void handleManualLoad();
    void showAudioSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
