#pragma once

#include <fstream>
#include <filesystem>
// #include <juce_core/files/juce_File.h>
#include "RTNeural.h"

namespace fs = std::filesystem;

const std::string js_fn = "models/rt_model_2024-05-28_11-54-32.json";
const std::string project_root = "wtianns_rtneural";

inline juce::File get_designated_plugin_path() {
    juce::File const app_data_dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    juce::File const wtianns_files_path = app_data_dir.getChildFile("nvssynthesis").getChildFile("wtianns");
    return wtianns_files_path;
}

constexpr int n_mfcc = 11;
constexpr int n_input = n_mfcc + 1;
constexpr int n_hidden = 128;
constexpr int n_output = 257;

constexpr double nn_sample_rate = 22050.0;

using ModelType = RTNeural::ModelT<float, n_input, n_output,
    RTNeural::GRULayerT<float, n_input, n_hidden>,
    RTNeural::GRULayerT<float, n_hidden, n_hidden>,
    RTNeural::DenseT<float, n_hidden, n_output>,
    RTNeural::ReLuActivationT<float, n_output>
>;

inline std::string getModelFilename(fs::path path) {
    // get path of RTNeural root directory
    while(path.filename() != project_root) {
        path = path.parent_path();
    }
    // get path of model file
    path.append(js_fn);

    return path.string();
}

inline void loadModel(std::ifstream& jsonStream, ModelType& model)
{
    nlohmann::json modelJson;
    jsonStream >> modelJson;

    auto& gru1 = model.get<0>();
    RTNeural::torch_helpers::loadGRU<float> (modelJson, "gru.", gru1);

    auto& dense = model.get<2>();
    RTNeural::torch_helpers::loadDense<float>(modelJson, "dense_layers.0.", dense);
}

template<typename sample_t>
[[nodiscard]] inline sample_t cubicInterp(sample_t const *const wavetable, sample_t  phase, int const winSize)
{// adapted from https://www.musicdsp.org/en/latest/Other/49-cubic-interpollation.html?highlight=cubic
    // assert(phase >= 0);
    phase = std::fmod(phase, 1.0);
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