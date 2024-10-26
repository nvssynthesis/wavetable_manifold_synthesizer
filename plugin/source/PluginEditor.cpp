#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
,  sliders_([&]() {
         std::vector<std::unique_ptr<AttachedSlider>> temp;
         auto constexpr N = params::to_idx( params::params_e::num_params );
         temp.reserve(N);
         for (int i = 0; i < N; ++i) {
             temp.emplace_back(std::make_unique<AttachedSlider>(p.getApvts(), params::from_idx(i)));
         }
         return temp;
     }())
{

    for (int i = 0; i < params::to_idx( params::params_e::num_params ); ++i) {
        addAndMakeVisible(sliders_[i]->slider_);
    }

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
    auto const width = getWidth() / params::to_idx(params::params_e::num_params);
    auto const height = getHeight();
    // freqSlider_.slider_.setBounds(x, y, width, height);
    for (auto &slider_ptr : sliders_) {
        slider_ptr->slider_.setBounds(x, y, width, height);
        x += width;
    }
}
