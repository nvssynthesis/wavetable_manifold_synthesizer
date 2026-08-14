#pragma once

#include <chowdsp_fft_juce/chowdsp_fft_juce.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "PhasedMultitrackWindowManager.h"
#include "../RTN/util.h"
#include "../params/params.h"
#include "fmt/base.h"

namespace nvs {




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

class WMS final : public juce::HighResolutionTimer {
	/*
	This class implements the wavetable manifold synthesis. Currently in minimum-working example form.
	*/
public:
	WMS();
    ~WMS() override;

    void hiResTimerCallback() override;

	void generateNextWaveform();

	void addLogger(juce::FileLogger *logger);
	void loadModel(juce::String const &modelFilePath);

	void resetProcessing(double sr, int samps_per_block);
	void processBlock (juce::AudioBuffer<float>& outputBuffer,  juce::MidiBuffer& midiMessages);

	void setFrequency(float newFrequency);
	void setVoicedness(float newVoicedness);
	void setCepstralCoefficient(params::params_e cc, float val);    // actually this could be used to set any member of synthesis array

private:

	std::array<float, static_cast<int>(params::params_e::num_params)> synthesis_params_array_ {
	    [] {
		    constexpr int N = static_cast<int>(params::params_e::num_params);
		    std::array<float, N> arr{};
		    for (int i=0; i < N; ++i) {
			    arr[i] = params::get_default<float>(params::from_idx(i));
		    }
		    return arr;
	    }()
	};

	using ModelType = nvs::rtn::ModelType;
	ModelType model_;

	static constexpr bool anti_alias_spectrum_ {false};	/// TODO

    static constexpr size_t wavelength { (ModelType::output_size-1) * 2 };

    class SwitchingBuffer {
    public:
        SwitchingBuffer();

        // the chanel currently being read by synthesis.
        [[nodiscard]]
        const float* getFreshReadPointer() const noexcept;

        // the channel that is safe to overwrite.
        [[nodiscard]]
        float* getStaleWritePointer() noexcept;

        // copy a complete waveform into the non-playing channel.
        // this does *NOT* make the new waveform active.
        void writeToStaleChannel(const float* source) noexcept;

        // make the previously-stale channel the new playing channel.
        // O(1): no sample data is copied.
        void promoteStaleToFresh() noexcept;

        [[nodiscard]]
        int getFreshChannelIndex() const noexcept;

        [[nodiscard]]
        int getStaleChannelIndex() const noexcept;

    private:
        juce::AudioBuffer<float> buffers;
        static constexpr int numChannels = 2;
        int freshChannel_ = 0;
        int staleChannel_ = 1;
    } switchingBuffer;

    class ScratchBuffer {
    public:
        ScratchBuffer();
        // FOR TIMER THREAD!
        // returns true if the ScratchBuffer is available for a new ANN prediction.
        [[nodiscard]]
        bool tryBeginWrite() const noexcept;

        // TIMER THREAD!
        // for after tryBeginWrite() succeeds and network has predicted outputs.
        void writeAndTransform(const float* source);

        // FOR AUDIO THREAD!
        // claims ready waveform.
        // upon success, the producer must not touch buff until this consumer has finished copying.
        [[nodiscard]]
        bool tryAcquireForRead() noexcept;

        [[nodiscard]]
        const float* getReadPointer() const noexcept;

    private:
        void normalize() {
            const int numSamples = buff.getNumSamples();

            const float magnitude =
                buff.getMagnitude(0, 0, numSamples);

            if (magnitude != 0.0f)
                buff.applyGain(1.0f / magnitude);
            else
                buff.clear();
        }

        juce::AudioBuffer<float> buff;
        std::atomic_bool readyToRead_ { false };
        juce::dsp::FFT fft_;

    } scratchBuff;

#if NOTNOW
	[[maybe_unused]] WavetableTransitionStrategy wt_transition_strategy_ {WavetableTransitionStrategy::finish_leftover_from_last_block_then_switch};
	/* This strategy may actually require more than 2 buffers, in case there is a wavelength lasting multiple buffers.
			In that case, the previous buffer should not be updated until the newest buffer is properly faded in.
			But >2 might be required in case e.g. one phase allows the new wave at t_1, but another phase allows the new wave at t_2.
	*/
    PhasedFourTrackWindowManager phased_hannings_;
#endif
    Phasor<double> phasor;

    double sample_rate_;
	int block_size_;

	juce::FileLogger *logger_ = nullptr;	// non-owning

};




} // namespace nvs