#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
freqSlider_(p.getApvts(), params::params_e::f0)
{
    addAndMakeVisible(freqSlider_.slider_);

    // freqSlider_.setSliderStyle(juce::Slider::SliderStyle::Rotary);
    // freqSlider_.setRange(5.0, 600.0);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (900, 300);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("WTIANNS with rtneural", getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto x = 0;
    auto y = 0;
    auto width = getWidth() / params::to_idx(params::params_e::num_params);
    auto height = getHeight();
    freqSlider_.slider_.setBounds(x, y, width, height);
    x += width;
}
