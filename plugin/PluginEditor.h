#pragma once

#include "PluginProcessor.h"
#include "GUI/AttachedSlider.h"

//==============================================================================
class WMSProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit WMSProcessorEditor (WMSAudioProcessor&);
    ~WMSProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    std::vector<std::unique_ptr<AttachedSlider>> sliders_;


    class GuiUpdateTimer final : public juce::Timer
    {
    public:
        GuiUpdateTimer(WMSProcessorEditor& e) : editor(e) {}
        void timerCallback() override
        {
            // for (int i = 0; i < params::num_params; ++i)
            // {
            //     const float val = editor.processorRef.currentParamNormalizedVals[i].load();
            //     editor.sliders_[i]->slider_.setValue(val, juce::dontSendNotification);
            // }
            for (int i = 0; i < params::num_params; ++i)
            {
                const float val = editor.wmsProcessor.currentParamNormalizedVals[i].load();
                auto& apvts = editor.wmsProcessor.getApvts();
                auto* param = apvts.getParameter(
                    params::get_param_id(
                        params::from_idx(i)));
                param->setValueNotifyingHost(val);
            }
        }
    private:
        WMSProcessorEditor& editor;
    } guiUpdateTimer;


    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    WMSAudioProcessor& wmsProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WMSProcessorEditor)
};
