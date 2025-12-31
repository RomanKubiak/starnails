#pragma once

#include <JuceHeader.h>

class ParameterGridComponent : public juce::Component,
                               private juce::AsyncUpdater,
                               private juce::Timer
{
public:
    ParameterGridComponent()
    {
        startTimerHz (30);
    }

    void setProcessor (juce::AudioProcessor* newProcessor)
    {
        processor = newProcessor;
        rebuildControls();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::transparentBlack);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        if (controls.empty())
            return;

        const int numItems = (int) controls.size();
        const int numCols = juce::jmax (1, (int) std::ceil (std::sqrt ((double) numItems)));
        const int numRows = (numItems + numCols - 1) / numCols;

        const int cellW = area.getWidth() / numCols;
        const int cellH = area.getHeight() / numRows;

        for (int i = 0; i < numItems; ++i)
        {
            const int row = i / numCols;
            const int col = i % numCols;
            controls[(size_t) i]->setBounds (area.getX() + col * cellW,
                                             area.getY() + row * cellH,
                                             cellW,
                                             cellH);
        }
    }

private:
    struct ParamControl : public juce::Component
    {
        explicit ParamControl (juce::AudioProcessorParameter& p) : param (p)
        {
            addAndMakeVisible (label);
            addAndMakeVisible (slider);

            label.setText (param.getName (64), juce::dontSendNotification);
            label.setJustificationType (juce::Justification::centred);

            slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
            slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
            slider.setRange (0.0, 1.0, 0.0);
            slider.setValue (param.getValue(), juce::dontSendNotification);

            slider.onValueChange = [this]
            {
                param.setValueNotifyingHost ((float) slider.getValue());
            };
        }

        void resized() override
        {
            auto area = getLocalBounds();
            auto labelHeight = juce::jmin (20, area.getHeight());
            label.setBounds (area.removeFromTop (labelHeight));
            slider.setBounds (area.reduced (4));
        }

        void syncFromParam()
        {
            slider.setValue (param.getValue(), juce::dontSendNotification);
        }

        juce::AudioProcessorParameter& param;
        juce::Slider slider;
        juce::Label label;
    };

    void rebuildControls()
    {
        cancelPendingUpdate();

        controls.clear();
        for (auto* child : getChildren())
            removeChildComponent (child);

        if (processor != nullptr)
        {
            for (auto* p : processor->getParameters())
            {
                auto c = std::make_unique<ParamControl> (*p);
                addAndMakeVisible (*c);
                controls.push_back (std::move (c));
            }
        }

        resized();
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        for (auto& c : controls)
            c->syncFromParam();
    }

    void timerCallback() override
    {
        triggerAsyncUpdate();
    }

    juce::AudioProcessor* processor { nullptr };
    std::vector<std::unique_ptr<ParamControl>> controls;
};
