#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <chowdsp_fft_juce/chowdsp_fft_juce.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "PhasedMultitrackWindowManager.h"
#include "util.h"

enum class WavetableTransitionStrategy {
    /*
     this strategy would look something like this
     |b0                  |b1
     /--w0--\/--w0--\/--w0--\/--w1--\
       /--w0--\/--w0--\/--w0--\/--w1--\
         /--w0--\/--w0--\/--w0--\/--w1--\
           /--w0--\/--w0--\/--w1--\/--w1--\
                           ^ distinctly the 1st window using the next wavetable, because its beginning falls on the next block
                           however, the windows that were already in progress get to complete using the previous wavetable
     */
    finish_leftover_from_last_block_then_switch = 0,
    /*
     this strategy looks rather like this
     |b0                  |b1                  |b2
     /^^w0^^\/--w0--\/__w0__\/__w2__\/--w2--\/^^w2^^\
       /^^w0^^\/--w0--\/__w0__\/__w2__\/^^w2^^\/^^w2^^\
         /^^w0^^\/--w0--\/__w0__\/--w2--\/^^w2^^\/^^w2^^\
           /^^w0^^\/--w0--\/__w2__\/--w2--\/^^w2^^\/^^w2^^\
     /__w1__\/--w1--\/^^w1^^\/^^w1^^\/--w1--\/__w1__\
        /__w1__\/--w1--\/^^w1^^\/--w1--\/__w1__\/__w3__\
          /__w1__\/--w1--\/^^w1^^\/--w1--\/__w1__\/__w3__\
            /__w1__\/--w1--\/^^w1^^\/--w1--\/__w1__\/__w3__\
     */
    fade_throughout_block = 1
};


//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState &getApvts() {
        return apvts_;
    }
private:
    ModelType model_;
    juce::dsp::FFT fft_;

    WavetableTransitionStrategy wt_transition_strategy_{WavetableTransitionStrategy::finish_leftover_from_last_block_then_switch};
    /* This strategy may actually require more than 2 buffers, in case there is a wavelength lasting multiple buffers.
        In that case, the previous buffer should not be updated until the newest buffer is properly faded in.
        But >2 might be required in case e.g. one phase allows the new wave at t_1, but another phase allows the new wave at t_2.
    */
    juce::AudioBuffer<float> wt_buff_prev_{juce::AudioBuffer<float>(1, (ModelType::output_size-1) * 2)};
    juce::AudioBuffer<float> wt_buff_curr_{juce::AudioBuffer<float>(1, (ModelType::output_size-1) * 2)};

    juce::AudioProcessorValueTreeState apvts_;
    // double f0_ {66.0};
    // juce::AudioParameterFloat *ap_f0_;
    PhasedFourTrackWindowManager phased_hannings_;

    // const juce::String logger_fp {get_designated_plugin_path()};
    juce::FileLogger logger_;
    juce::WavAudioFormat wav_audio_format_;
    std::unique_ptr<juce::AudioFormatWriter> audio_format_writer_;
    bool wav_written_ {false};


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();