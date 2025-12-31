#pragma once

#include <JuceHeader.h>

class SinglePluginLoader
{
public:
    SinglePluginLoader()
    {
    }

    ~SinglePluginLoader()
    {
        unload();
    }

    bool load (const juce::File& pluginFile, double sampleRate, int blockSize, juce::String& errorMessage)
    {
        unload();
        ensureFormats();

        if (! pluginFile.existsAsFile())
        {
            errorMessage = "Plugin file does not exist: " + pluginFile.getFullPathName();
            return false;
        }

        std::unique_ptr<juce::PluginDescription> description;
        juce::AudioPluginFormat* owningFormat = nullptr;

        for (int i = 0; i < formatManager.getNumFormats(); ++i)
        {
            auto* format = formatManager.getFormat (i);
            juce::OwnedArray<juce::PluginDescription> types;
            format->findAllTypesForFile (types, pluginFile.getFullPathName());

            if (! types.isEmpty())
            {
                description.reset (types.removeAndReturn (0));
                owningFormat = format;
                break;
            }
        }

        if (description == nullptr || owningFormat == nullptr)
        {
            errorMessage = "No compatible plugin format for file: " + pluginFile.getFullPathName();
            return false;
        }

        pluginInstance = owningFormat->createInstanceFromDescription (*description, sampleRate, blockSize, errorMessage);

        if (pluginInstance == nullptr)
        {
            if (errorMessage.isEmpty())
                errorMessage = "Failed to create plugin instance.";

            return false;
        }

        errorMessage = {};
        return true;
    }

    void unload()
    {
        pluginInstance.reset();
    }

    juce::AudioPluginInstance* get() const noexcept { return pluginInstance.get(); }
    bool isLoaded() const noexcept { return pluginInstance != nullptr; }

private:
    void ensureFormats()
    {
        if (formatsInitialised)
            return;

        formatsInitialised = true;

       #if JUCE_PLUGINHOST_VST
        formatManager.addFormat (new juce::VSTPluginFormat());
       #endif
       #if JUCE_PLUGINHOST_VST3
        formatManager.addFormat (new juce::VST3PluginFormat());
       #endif
    }

    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> pluginInstance;
    bool formatsInitialised { false };
};
