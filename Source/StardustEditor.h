#pragma once

#include <JuceHeader.h>

//==============================================================================
class StardustEditor : public juce::Component,
                       private juce::AsyncUpdater,
                       private juce::Timer
{
public:
    StardustEditor();
    ~StardustEditor() override;

    void setProcessor (juce::AudioProcessor* newProcessor);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    struct ParameterControl : public juce::Component
    {
        ParameterControl (juce::AudioProcessorParameter& p, const juce::String& labelOverride = {});
        
        void paint (juce::Graphics& g) override;
        void resized() override;
        void syncFromParam();

        juce::AudioProcessorParameter& param;
        juce::Slider slider;
        juce::Label nameLabel;
        juce::Label valueLabel;
    };

    //==============================================================================
    struct GroupComponent : public juce::Component
    {
        GroupComponent (const juce::String& name);
        
        void paint (juce::Graphics& g) override;
        void resized() override;
        
        juce::String groupName;
        std::vector<std::unique_ptr<ParameterControl>> parameters;
        std::vector<std::unique_ptr<GroupComponent>> subGroups;
    };

    //==============================================================================
    void rebuildInterface();
    void handleAsyncUpdate() override;
    void timerCallback() override;

    void createControlsForGroup (const juce::AudioProcessorParameterGroup& group, GroupComponent& comp);
    void organizeParametersSmartly (const juce::Array<juce::AudioProcessorParameter*>& params, GroupComponent& root);

    juce::AudioProcessor* processor = nullptr;
    std::unique_ptr<GroupComponent> rootGroup;
    juce::Image bgImage;
    
    // Flat list for quick updates
    std::vector<ParameterControl*> allControls;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StardustEditor)
};
