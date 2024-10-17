#pragma once

#include <fstream>
#include <filesystem>
#include "RTNeural.h"

namespace fs = std::filesystem;

const std::string js_fn = "models/rt_model_2024-05-28_11-54-32.json";
const std::string project_root = "wtianns_rtneural";

inline juce::File get_designated_plugin_path() {
    juce::File const app_data_dir = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory);
    juce::File const wtianns_files_path = app_data_dir.getChildFile("nvssynthesis").getChildFile("wtianns");
    return wtianns_files_path;
}

constexpr int n_mfcc = 12;
constexpr int n_input = n_mfcc + 1;
constexpr int n_hidden_1 = 80;
constexpr int n_hidden_2 = 128;
constexpr int n_output = 257;

using ModelType = RTNeural::ModelT<float, n_input, n_output,
    RTNeural::DenseT<float, n_input, n_hidden_1>,
    RTNeural::ReLuActivationT<float, n_hidden_1>,
    RTNeural::DenseT<float, n_hidden_1, n_hidden_2>,
    RTNeural::ReLuActivationT<float, n_hidden_2>,
    RTNeural::DenseT<float, n_hidden_2, n_output>>;

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

    auto& input = model.get<0>();
    RTNeural::torch_helpers::loadDense<float> (modelJson, "dense_layers.0.", input);

    auto& hidden_1 = model.get<2>();
    RTNeural::torch_helpers::loadDense<float> (modelJson, "dense_layers.2.", hidden_1);

    auto& hidden_2 = model.get<4>();
    RTNeural::torch_helpers::loadDense<float> (modelJson, "dense_layers.4.", hidden_2);
}
