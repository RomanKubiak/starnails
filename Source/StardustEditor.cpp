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

    // Neon 80s palette for contrast
    const auto trackColour   = juce::Colour::fromRGB (0x1a, 0x10, 0x33);   // deep indigo track
    const auto fillColour    = juce::Colour::fromRGB (0xff, 0x2f, 0xd0);   // magenta fill
    const auto thumbColour   = juce::Colour::fromRGB (0x27, 0xe8, 0xff);   // cyan thumb
    const auto valueText     = juce::Colour::fromRGB (0x74, 0xf0, 0xff);   // bright aqua text

    slider.setColour (juce::Slider::backgroundColourId, trackColour);
    slider.setColour (juce::Slider::trackColourId, fillColour);
    slider.setColour (juce::Slider::thumbColourId, thumbColour);

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
    valueLabel.setColour (juce::Label::textColourId, valueText);

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

    const auto panel     = juce::Colour::fromRGB (0x1a, 0x10, 0x33);
    const auto accentM   = juce::Colour::fromRGB (0xff, 0x2f, 0xd0);
    const auto accentC   = juce::Colour::fromRGB (0x27, 0xe8, 0xff);
    const auto textMain  = juce::Colour::fromRGB (0xf4, 0xf8, 0xff);

    g.setColour (panel.withAlpha (0.35f));
    g.fillRoundedRectangle (bounds.reduced (2.0f), 6.0f);

    g.setColour (accentM.withAlpha (0.4f));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 6.0f, 1.5f);

    if (groupName.isNotEmpty())
    {
        auto headerBounds = bounds.removeFromTop (30.0f);
        juce::ColourGradient grad (accentM.withAlpha (0.28f), headerBounds.getX(), headerBounds.getY(),
                                   accentC.withAlpha (0.24f), headerBounds.getRight(), headerBounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (headerBounds.reduced (3.0f), 5.0f);

        g.setColour (textMain);
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
        flex.alignContent = juce::FlexBox::AlignContent::flexStart;
        flex.alignItems = juce::FlexBox::AlignItems::flexStart;
    }
    else
    {
        // Leaf group with parameters: vertical stack
        flex.flexDirection = juce::FlexBox::Direction::column;
        const bool isMultiLeaf = groupName.containsIgnoreCase ("Multiband") || groupName.containsIgnoreCase ("Wideband");
        flex.alignContent = isMultiLeaf ? juce::FlexBox::AlignContent::flexStart
                                        : juce::FlexBox::AlignContent::stretch;
        flex.justifyContent = isMultiLeaf ? juce::FlexBox::JustifyContent::flexStart
                                          : juce::FlexBox::JustifyContent::center;
    }
    
    const int itemHeight = 26;
    const int groupSpacing = 10;

    for (auto& sub : subGroups)
    {
        auto margin = juce::FlexItem::Margin (groupSpacing);
        if (sub->groupName.containsIgnoreCase ("Multiband") || sub->groupName.containsIgnoreCase ("Wideband"))
            margin.top = juce::jmax (0.0f, margin.top - 8.0f);

        juce::FlexItem item (*sub);
        item.flexGrow = 1.0f;
        const bool isMulti = sub->groupName.containsIgnoreCase ("Multiband") || sub->groupName.containsIgnoreCase ("Wideband");
        const bool isStereoEnh = sub->groupName.containsIgnoreCase ("Stereo Enhancer");
        const bool isBassEnh = sub->groupName.containsIgnoreCase ("Bass Enhancer") || sub->groupName.containsIgnoreCase ("Bass");
        const bool isTrebleEnh = sub->groupName.containsIgnoreCase ("Treble Enhancer") || sub->groupName.containsIgnoreCase ("Treble");
        const bool compact = isStereoEnh || isBassEnh || isTrebleEnh;
        const bool compactEnh = compact || isBassEnh || isTrebleEnh;
        item.minWidth = compactEnh ? 190.0f : 300.0f;
        item.minHeight = compactEnh ? 130.0f : 250.0f;
         item.margin = margin;

        flex.items.add (item);
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
    auto bounds = getLocalBounds().toFloat();

    if (bgImage.isValid())
    {
        g.drawImage (bgImage, bounds, juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        juce::Colour base    = juce::Colour::fromRGB (0x0b, 0x10, 0x18);
        juce::Colour panel   = juce::Colour::fromRGB (0x1a, 0x10, 0x33);
        juce::Colour accentM = juce::Colour::fromRGB (0xff, 0x2f, 0xd0);
        juce::Colour accentC = juce::Colour::fromRGB (0x27, 0xe8, 0xff);

        juce::ColourGradient gradient { panel, bounds.getTopLeft(),
                                        base,  bounds.getBottomRight(), false };
        gradient.addColour (0.35f, accentM.withAlpha (0.12f));
        gradient.addColour (0.65f, accentC.withAlpha (0.10f));

        g.setGradientFill (gradient);
        g.fillRect (bounds);
    }
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
        { "Wideband Compressor", { "Wideband", "Master", "Comp", "" }, { // Empty string as fallback to match generic names
            { "Input gain", { "Input", "In Gain" } },
            { "Threshold", { "Thresh" } },
            { "Ratio", { "Ratio" } },
            { "Attack", { "Attack" } },
            { "Release", { "Release" } },
            { "Output Gain", { "Output", "Out Gain", "MakeUp" } }
        }},
        { "Multiband Compressor", { "Multi", "MB", "Mid", "" }, {
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
