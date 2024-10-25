#pragma once
#include <JuceHeader.h>
#include "params.h"

struct AttachedSlider {
    using Slider = juce::Slider;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    AttachedSlider(juce::AudioProcessorValueTreeState &apvts,
        params::params_e const param,
        Slider::SliderStyle const sliderStyle = Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition const entryPos = juce::Slider::TextBoxBelow)
    // AttachedSlider(juce::RangedAudioParameter* rap,
    //     Slider::SliderStyle  style = Slider::SliderStyle::RotaryHorizontalVerticalDrag,
    //     juce::Slider::TextEntryBoxPosition  entryPos = juce::Slider::TextBoxBelow)
    :
    slider_(sliderStyle, entryPos),
    attachment_(apvts, params::get_param_id(param), slider_)
    {
        slider_.setSliderStyle(sliderStyle);
        slider_.setNormalisableRange(/*getNormalizableRange<double>(param)*/ juce::NormalisableRange<double>(0.0, 1.0));
        slider_.setTextBoxStyle(entryPos, false, 50, 25);
        slider_.setValue(/*getParamDefault(param)*/ 0.0);

        slider_.setColour(Slider::ColourIds::thumbColourId, juce::Colours::palevioletred);
        slider_.setColour(Slider::ColourIds::textBoxTextColourId, juce::Colours::lightgrey);
    }

    Slider slider_;
    SliderAttachment attachment_;
};
