#include "StardustEditor.h"

//==============================================================================
StardustEditor::ParameterControl::ParameterControl (juce::AudioProcessorParameter& p, const juce::String& labelOverride) : param (p)
{
    addAndMakeVisible (slider);
    addAndMakeVisible (nameLabel);
    addAndMakeVisible (valueLabel);

    // Configure Slider
    slider.setSliderStyle (juce::Slider::LinearBar);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (0.0, 1.0, 0.0);
    
    // We use the normalized value for the slider, but display the formatted text
    slider.onValueChange = [this]
    {
        if (slider.isMouseButtonDown())
            param.setValueNotifyingHost ((float) slider.getValue());
    };
    
    slider.onDragStart = [this] { param.beginChangeGesture(); };
    slider.onDragEnd   = [this] { param.endChangeGesture(); };

    // Configure Labels
    nameLabel.setText (labelOverride.isNotEmpty() ? labelOverride : param.getName (128), juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centredLeft);
    nameLabel.setFont (18.0f);
    nameLabel.setInterceptsMouseClicks (false, false);
    nameLabel.setColour (juce::Label::textColourId, juce::Colours::white);

    valueLabel.setJustificationType (juce::Justification::centredRight);
    valueLabel.setFont (18.0f);
    valueLabel.setInterceptsMouseClicks (false, false);
    valueLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    
    syncFromParam();
}

void StardustEditor::ParameterControl::paint (juce::Graphics& g)
{
    // Optional: custom background for parameter row
}

void StardustEditor::ParameterControl::resized()
{
    auto area = getLocalBounds();
    slider.setBounds (area);
    
    auto labelArea = area.reduced (10, 0);
    nameLabel.setBounds (labelArea.removeFromLeft (labelArea.getWidth() / 2));
    valueLabel.setBounds (labelArea);
}

void StardustEditor::ParameterControl::syncFromParam()
{
    slider.setValue (param.getValue(), juce::dontSendNotification);
    valueLabel.setText (param.getCurrentValueAsText(), juce::dontSendNotification);
}

//==============================================================================
StardustEditor::GroupComponent::GroupComponent (const juce::String& name) : groupName (name)
{
}

void StardustEditor::GroupComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Draw group frame
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 4.0f, 1.0f);
    
    // Draw group name background
    if (groupName.isNotEmpty())
    {
        auto headerBounds = bounds.removeFromTop (30.0f);
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.fillRoundedRectangle (headerBounds.reduced (2.0f), 4.0f);
        
        g.setColour (juce::Colours::white);
        g.setFont (20.0f);
        g.drawText (groupName, headerBounds, juce::Justification::centred, true);
    }
}

void StardustEditor::GroupComponent::resized()
{
    auto area = getLocalBounds().reduced (6);
    
    if (groupName.isNotEmpty())
        area.removeFromTop (30); // Space for header

    juce::FlexBox flex;
    
    // If we have subgroups, we likely want a grid/wrap layout (dashboard style)
    if (! subGroups.empty())
    {
        flex.flexDirection = juce::FlexBox::Direction::row;
        flex.flexWrap = juce::FlexBox::Wrap::wrap;
        flex.alignContent = juce::FlexBox::AlignContent::stretch;
    }
    else
    {
        // Leaf group with parameters: vertical stack
        flex.flexDirection = juce::FlexBox::Direction::column;
        flex.alignContent = juce::FlexBox::AlignContent::stretch;
        flex.justifyContent = juce::FlexBox::JustifyContent::center;
    }
    
    const int itemHeight = 45;
    const int groupSpacing = 10;

    for (auto& sub : subGroups)
    {
        // Give subgroups a minimum width so they wrap nicely
        flex.items.add (juce::FlexItem (*sub).withFlex (1.0f).withMargin (groupSpacing).withMinWidth (300.0f).withMinHeight (250.0f));
    }
    
    for (auto& param : parameters)
    {
        flex.items.add (juce::FlexItem (*param).withHeight (itemHeight).withMargin (2));
    }
    
    flex.performLayout (area);
}

//==============================================================================
StardustEditor::StardustEditor()
{
    setSize (1024, 600);
    startTimerHz (30); // Update UI at 30fps

    // Load background image
    juce::File exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    juce::File imgFile = exe.getParentDirectory().getChildFile ("Resources").getChildFile ("stardust.png");
    if (imgFile.existsAsFile())
        bgImage = juce::ImageFileFormat::loadFrom (imgFile);
}

StardustEditor::~StardustEditor()
{
    stopTimer();
}

void StardustEditor::setProcessor (juce::AudioProcessor* newProcessor)
{
    processor = newProcessor;
    triggerAsyncUpdate();
}

void StardustEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (11, 16, 24)); // Match main background
    if (bgImage.isValid())
        g.drawImage (bgImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void StardustEditor::resized()
{
    if (rootGroup)
    {
        rootGroup->setBounds (getLocalBounds().reduced (10));
    }
}

void StardustEditor::handleAsyncUpdate()
{
    rebuildInterface();
}

void StardustEditor::timerCallback()
{
    for (auto* ctrl : allControls)
        if (ctrl != nullptr)
            ctrl->syncFromParam();
}

void StardustEditor::rebuildInterface()
{
    rootGroup.reset();
    allControls.clear();

    if (processor == nullptr)
        return;

    rootGroup = std::make_unique<GroupComponent> (""); // Root has no name
    addAndMakeVisible (rootGroup.get());

    auto& tree = processor->getParameterTree();
    
    // Heuristic: If the tree is flat (only parameters, no subgroups), try smart grouping
    bool isFlat = true;
    for (auto* node : tree)
    {
        if (node->getGroup() != nullptr)
        {
            isFlat = false;
            break;
        }
    }

    if (isFlat)
    {
        organizeParametersSmartly (processor->getParameters(), *rootGroup);
    }
    else
    {
        createControlsForGroup (tree, *rootGroup);
    }

    resized();
}

void StardustEditor::createControlsForGroup (const juce::AudioProcessorParameterGroup& group, GroupComponent& comp)
{
    for (auto* node : group)
    {
        if (auto* param = node->getParameter())
        {
            auto ctrl = std::make_unique<ParameterControl> (*param);
            comp.addAndMakeVisible (ctrl.get());
            allControls.push_back (ctrl.get());
            comp.parameters.push_back (std::move (ctrl));
        }
        else if (auto* subGroup = node->getGroup())
        {
            auto subComp = std::make_unique<GroupComponent> (subGroup->getName());
            createControlsForGroup (*subGroup, *subComp);
            comp.addAndMakeVisible (subComp.get());
            comp.subGroups.push_back (std::move (subComp));
        }
    }
}

void StardustEditor::organizeParametersSmartly (const juce::Array<juce::AudioProcessorParameter*>& params, GroupComponent& root)
{
    std::vector<juce::AudioProcessorParameter*> availableParams;
    for (auto* p : params) availableParams.push_back (p);

    struct ParamRule {
        juce::String label;
        std::vector<juce::String> searchKeys;
    };

    struct GroupRule {
        juce::String title;
        std::vector<juce::String> groupKeys;
        std::vector<ParamRule> paramRules;
    };

    std::vector<GroupRule> rules = {
        { "Bass Enhancer", { "Bass", "Low", "" }, {
            { "Phase rotator", { "Phase", "Rotator" } },
            { "Frequency", { "Freq", "Hz" } },
            { "Q", { "Q", "Res" } },
            { "Gain", { "Gain", "Level" } }
        }},
        { "Treble Enhancer", { "Treble", "High", "" }, {
            { "Phase Rotator", { "Phase", "Rotator" } },
            { "Frequency", { "Freq", "Hz" } },
            { "Q", { "Q", "Res" } },
            { "Gain", { "Gain", "Level" } }
        }},
        { "Stereo Enhancer", { "Stereo", "Width", "" }, {
            { "Wide Coef", { "Wide", "Width", "Coef" } },
            { "Delay", { "Delay" } }
        }},
        { "Multiband Compressor", { "Multi", "MB", "Mid", "" }, {
            { "Input gain", { "Input", "In Gain" } },
            { "Threshold", { "Thresh" } },
            { "Ratio", { "Ratio" } },
            { "Attack", { "Attack" } },
            { "Release", { "Release" } },
            { "Output Gain", { "Output", "Out Gain", "MakeUp" } }
        }},
        { "Wideband Compressor", { "Wideband", "Master", "Comp", "" }, { // Empty string as fallback to match generic names
            { "Input gain", { "Input", "In Gain" } },
            { "Threshold", { "Thresh" } },
            { "Ratio", { "Ratio" } },
            { "Attack", { "Attack" } },
            { "Release", { "Release" } },
            { "Output Gain", { "Output", "Out Gain", "MakeUp" } }
        }}
    };

    for (const auto& groupRule : rules)
    {
        auto subComp = std::make_unique<GroupComponent> (groupRule.title);
        bool hasParams = false;

        for (const auto& paramRule : groupRule.paramRules)
        {
            // Find best match
            auto it = std::find_if (availableParams.begin(), availableParams.end(), [&] (juce::AudioProcessorParameter* p)
            {
                juce::String name = p->getName (128);
                
                // Check group keys
                bool groupMatch = false;
                for (const auto& k : groupRule.groupKeys)
                {
                    if (k.isEmpty() || name.containsIgnoreCase (k))
                    {
                        groupMatch = true;
                        break;
                    }
                }
                if (! groupMatch) return false;

                // Check param keys
                for (const auto& k : paramRule.searchKeys)
                {
                    if (name.containsIgnoreCase (k)) return true;
                }
                return false;
            });

            if (it != availableParams.end())
            {
                auto* p = *it;
                availableParams.erase (it);
                
                auto ctrl = std::make_unique<ParameterControl> (*p, paramRule.label);
                subComp->addAndMakeVisible (ctrl.get());
                allControls.push_back (ctrl.get());
                subComp->parameters.push_back (std::move (ctrl));
                hasParams = true;
            }
        }

        if (hasParams)
        {
            root.addAndMakeVisible (subComp.get());
            root.subGroups.push_back (std::move (subComp));
        }
    }
}
