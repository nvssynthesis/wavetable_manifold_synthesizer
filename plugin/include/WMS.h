#pragma once

#include <chowdsp_fft_juce/chowdsp_fft_juce.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "PhasedMultitrackWindowManager.h"
#include "util.h"
#include "params.h"

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

	class WMS {
		/*
		This class implements the wavetable manifold synthesis. Currently in minimum-working example form.
		*/
	public:
		WMS();
		void addLogger(juce::FileLogger *logger);
		void loadModel(juce::String const &modelFilePath);

		void reset (double sr, int samps_per_block);
		void processBlock (juce::AudioBuffer<float>& outputBuffer,  juce::MidiBuffer& midiMessages);

		void setFrequency(float newFrequency);
		void setVoicedness(float newVoicedness);
		void setCepstralCoefficients(float cc0, float cc1, float cc2);

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
		juce::dsp::FFT fft_;

		static constexpr bool anti_alias_spectrum_ {false};
		size_t counter_ = 0;
		size_t const counter_lim_ = 1;

		[[maybe_unused]] WavetableTransitionStrategy wt_transition_strategy_ {WavetableTransitionStrategy::finish_leftover_from_last_block_then_switch};
		/* This strategy may actually require more than 2 buffers, in case there is a wavelength lasting multiple buffers.
				In that case, the previous buffer should not be updated until the newest buffer is properly faded in.
				But >2 might be required in case e.g. one phase allows the new wave at t_1, but another phase allows the new wave at t_2.
		*/
		juce::AudioBuffer<float> wt_buff_prev_{juce::AudioBuffer<float>(1, (ModelType::output_size-1) * 2)};
		juce::AudioBuffer<float> wt_buff_curr_{juce::AudioBuffer<float>(1, (ModelType::output_size-1) * 2)};
		PhasedFourTrackWindowManager phased_hannings_;

		double sample_rate_;
		int block_size_;

		juce::FileLogger *logger_ = nullptr;	// non-owning
	};
} // namespace nvs