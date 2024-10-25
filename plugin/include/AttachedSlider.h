#pragma once
#include <JuceHeader.h>
#include "params.h"

struct AttachedSlider {
    using Slider = juce::Slider;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    AttachedSlider(juce::AudioProcessorValueTreeState &apvts,
        params::params_e param,
        Slider::SliderStyle sliderStyle = Slider::SliderStyle::LinearBarVertical,
        juce::Slider::TextEntryBoxPosition entryPos = juce::Slider::TextBoxBelow);

    Slider slider_;
    SliderAttachment attachment_;
};
