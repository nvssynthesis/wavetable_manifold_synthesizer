#pragma once
#include <fstream>
#include <filesystem>
#include "RTNeural.h"

namespace nvs {

    inline juce::File get_designated_plugin_path() {
        juce::File const app_data_dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
        juce::File const wtianns_files_path = app_data_dir.getChildFile("nvssynthesis").getChildFile("wtianns");
        return wtianns_files_path;
    }

    namespace rtn {
        namespace fs = std::filesystem;

        const std::string js_rtneural_fn = "models/rt_model_2024-11-20_20-51-12.json";
        const std::string project_root = "wtianns_rtneural";

        constexpr bool include_voicedness = true;
        constexpr bool include_frequency = true;
        constexpr int n_pitch = static_cast<int>(include_frequency) + static_cast<int>(include_voicedness);

        constexpr int n_mfcc = 11;
        constexpr int n_mfcc_dim_reduced = 3;
        constexpr int n_input = n_mfcc_dim_reduced + n_pitch;

        constexpr int n_encoded = n_mfcc + n_pitch;

        constexpr int n_hidden = 132;
        constexpr int n_output = 513;

        constexpr float F_MIN_DETECTED = 46.f;

        constexpr double nn_sample_rate = 16000.0;

        using ModelType = RTNeural::ModelT<float, n_input, n_output,
            RTNeural::DenseT<float, n_input, n_encoded>,
            RTNeural::ReLuActivationT<float, n_encoded>,
            RTNeural::GRULayerT<float, n_encoded, n_hidden>,
            RTNeural::DenseT<float, n_hidden, n_output>,
            RTNeural::ReLuActivationT<float, n_output>
        >;

        inline juce::String getModelFilename() {
            auto const plugin_special_path = nvs::get_designated_plugin_path();
            auto const model_path = plugin_special_path.getChildFile(js_rtneural_fn);
            return model_path.getFullPathName();
        }

        inline void loadModel(std::ifstream& jsonStream, ModelType& model)
        {
            nlohmann::json modelJson;
            jsonStream >> modelJson;

            auto& in_encoder = model.get<0>();
            RTNeural::torch_helpers::loadDense<float>(modelJson, "input_encoder.0.", in_encoder);

            auto& gru0 = model.get<2>();
            RTNeural::torch_helpers::loadGRU<float> (modelJson, "gru.", gru0);

            auto& dense = model.get<3>();
            RTNeural::torch_helpers::loadDense<float>(modelJson, "dense_layers.0.", dense);
        }
    }   // namespace rtn


    template<typename sample_t>
    [[nodiscard]] inline sample_t cubicInterp(sample_t const *const wavetable, sample_t  phase, int const winSize)
    {// adapted from https://www.musicdsp.org/en/latest/Other/49-cubic-interpollation.html?highlight=cubic
        // assert(phase >= 0);
        // phase = std::fmod(phase, 1.0);  // if i can get away with taking this out, do it
        assert(phase <= 1);
        sample_t fIdx = phase * winSize;
        int const iIdx = static_cast<int>(fIdx);// assuming we never get a negative fIdx; then it would round up
        sample_t const frac = fIdx - static_cast<sample_t>(iIdx);
        auto m1 = (iIdx - 1) & (winSize - 1);
        auto p2 = (iIdx + 2) & (winSize - 1);
        sample_t const ym1 = wavetable[m1];    // could &= the mask here
        sample_t const y0  = wavetable[(iIdx + 0) & (winSize - 1)];
        sample_t const y1  = wavetable[(iIdx + 1) & (winSize - 1)];    // and here...
        sample_t const y2  = wavetable[p2];    // and here...
        sample_t const a = (3.f * (y0-y1) - ym1 + y2) * 0.5f;
        sample_t const b = 2.f*y1 + ym1 - (5.f*y0 + y2) * 0.5f;
        sample_t const c = (y1 - ym1) * 0.5f;
        sample_t const y = (((a * frac) + b) * frac + c) * frac + y0;
        return y;
    }
    inline float pitchLinearToLogScale(float frequency, float const f_min= rtn::F_MIN_DETECTED, float const eps=0.0001f) {
        /*
         Since the pitch was mapped to a log scale for training, we need to use that mapping when feeding it to the network
         while leaving it unchanged for the actual sounding frequency.
         */
        assert (f_min > 0.f);
        frequency = frequency >= f_min ? frequency : f_min;
        return std::logf(frequency - f_min + eps);
    }
}   // namespace nvs