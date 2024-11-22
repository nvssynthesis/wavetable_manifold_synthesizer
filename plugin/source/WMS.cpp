#include "WMS.h"
#include "fmt/base.h"

namespace nvs {
    WMS::WMS() :
        fft_(static_cast<int>(std::log2((ModelType::output_size-1) * 2))),
        wt_buff_prev_(1, (ModelType::output_size-1) * 2),
        wt_buff_curr_(1, (ModelType::output_size-1) * 2) {

    }

    void WMS::addLogger(juce::FileLogger *logger) {
        logger_ = logger;
    }

    void WMS::loadModel(juce::String const &modelFilePath) {
        std::ifstream jsonStream(modelFilePath.toStdString(), std::ifstream::binary);
        nvs::rtn::loadModel(jsonStream, this->model_);
        if (logger_) {
            logger_->logMessage("Model loaded from " + modelFilePath);
        }
    }
    void WMS::reset (double sr, int samps_per_block) {
        sample_rate_ = sr;
        block_size_ = samps_per_block;
        model_.reset();
    }

    void WMS::setFrequency(float newFrequency) {
        synthesis_params_array_[params::to_idx(params::params_e::f0)] = newFrequency;
    }
    void WMS::setVoicedness(float newVoicedness) {
        synthesis_params_array_[params::to_idx(params::params_e::voicedness)] = newVoicedness;
    }
    void WMS::setCepstralCoefficients(float cc0, float cc1, float cc2) {
        synthesis_params_array_[params::to_idx(params::params_e::cc0)] = cc0;
        synthesis_params_array_[params::to_idx(params::params_e::cc1)] = cc1;
        synthesis_params_array_[params::to_idx(params::params_e::cc2)] = cc2;
    }

    void WMS::processBlock (juce::AudioBuffer<float>& outputBuffer,
                                              juce::MidiBuffer& midiMessages) {
        juce::ignoreUnused (midiMessages);

        int const wavelength = wt_buff_curr_.getNumSamples();
        auto f0_val = synthesis_params_array_[params::to_idx(params::params_e::f0)];
        if (counter_ == 0) {

            // this alignment prevented runtime error in Eigen
            alignas(16) const std::array<float, params::to_idx(params::params_e::num_params)> inputs {
                synthesis_params_array_[params::to_idx(params::params_e::cc0)],
                synthesis_params_array_[params::to_idx(params::params_e::cc1)],
                synthesis_params_array_[params::to_idx(params::params_e::cc2)],
                                            nvs::pitchLinearToLogScale(f0_val),
                synthesis_params_array_[params::to_idx(params::params_e::voicedness)]};

            model_.forward(&inputs[0]);
            wt_buff_curr_.copyFrom(0, 0, model_.getOutputs(), ModelType::output_size);
            if (std::isnan(wt_buff_curr_.getSample(0, 0))) {
                fmt::print("buffer contains NaN");
                logger_->logMessage("buffer contains NaN");
                return;
            }
            if constexpr (anti_alias_spectrum_) {
                // this functionality currently removes all energy above the FUNDAMENTAL, not above where the highest ALLOWED bin should be.
                // determine highest bin that should have nonzero energy
                int highest_bin = static_cast<int>(ModelType::output_size);
                double model_sr = nvs::rtn::nn_sample_rate;

                auto const freq_frac_of_nyquist = f0_val / (sample_rate_ / 2);
                auto const highest_allowed_bin = static_cast<int>(freq_frac_of_nyquist * highest_bin);
                jassert(highest_allowed_bin <= highest_bin);
                wt_buff_curr_.clear(highest_allowed_bin, highest_bin-highest_allowed_bin);
            }
            fft_.performRealOnlyInverseTransform(wt_buff_curr_.getWritePointer(0));


            // normalize
            bool const normalize = true;
            if (normalize) {
                float mag = wt_buff_curr_.getMagnitude(0, 0, wavelength);
                if (mag == 0.f) {
                    mag = 1.f;
                }
                wt_buff_curr_.applyGain(1.f / mag);
            }

            phased_hannings_.allowTransition();
        }
        ++counter_;
        if (counter_ == counter_lim_) {
            counter_ = 0;
        }
        for (int samp_idx = 0; samp_idx < block_size_; ++samp_idx) {
            auto wins_and_phases = phased_hannings_.calculateWindowAndPhase();
            float samp = 0;
            for (auto & wins_and_phase : wins_and_phases){
                float samp_tmp = nvs::cubicInterp(wt_buff_curr_.getReadPointer(0),
                    static_cast<float>(wins_and_phase.phase_), wavelength);
                samp_tmp *= static_cast<float>(wins_and_phase.window_per_waveform_[0]);
                samp += samp_tmp;

                samp_tmp = nvs::cubicInterp(wt_buff_prev_.getReadPointer(0),
                    static_cast<float>(wins_and_phase.phase_), wavelength);
                samp_tmp *= static_cast<float>(wins_and_phase.window_per_waveform_[1]);
                samp += samp_tmp;
            }
            samp = juce::jlimit(-1.f, 1.f, samp);
            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel) {

                auto* channelData = outputBuffer.getWritePointer (channel);
                channelData[samp_idx] = samp * 0.9f;
            }
            phased_hannings_.increment_phase(f0_val / sample_rate_);
        }
        wt_buff_prev_ = wt_buff_curr_;
    }

}