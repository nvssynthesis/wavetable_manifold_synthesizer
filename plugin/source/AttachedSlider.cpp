#pragma once
#include "AttachedSlider.h"

AttachedSlider::AttachedSlider(juce::AudioProcessorValueTreeState &apvts,
        params::params_e const param,
        Slider::SliderStyle const sliderStyle,
        juce::Slider::TextEntryBoxPosition const entryPos)
:
slider_(sliderStyle, entryPos),
attachment_(apvts, params::get_param_id(param), slider_)
{
    slider_.setSliderStyle(sliderStyle);
    slider_.setNormalisableRange(params::get_normalizable_range<double>(param));
    slider_.setTextBoxStyle(entryPos, false, 50, 25);
    slider_.setValue(params::get_default<double>(param));

    slider_.setColour(Slider::ColourIds::thumbColourId, juce::Colours::palevioletred);
    slider_.setColour(Slider::ColourIds::textBoxTextColourId, juce::Colours::lightgrey);
}

