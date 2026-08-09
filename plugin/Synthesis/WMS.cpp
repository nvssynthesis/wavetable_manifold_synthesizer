#include "WMS.h"

namespace nvs {
WMS::WMS()
{
    startTimer(10.0);
}

WMS::~WMS() {
    stopTimer();
}


void WMS::hiResTimerCallback() {

    static int iter = 0;
    if (iter++ < 3) {
        return;
    }

    generateNextWaveform();
}

void WMS::loadModel(juce::String const &modelFilePath) {
    std::ifstream jsonStream(modelFilePath.toStdString(), std::ifstream::binary);
    nvs::rtn::loadModel(jsonStream, this->model_);
    if (logger_) {
        logger_->logMessage("Model loaded from " + modelFilePath);
    }
}
void WMS::resetProcessing(const double sr, const int samps_per_block) {
    sample_rate_ = sr;
    block_size_ = samps_per_block;
    model_.reset();
    phasor.setSampleRate(sr);
}

void WMS::generateNextWaveform() {
    if (!scratchBuff.tryBeginWrite()) {
        return; // don't even run RTNeural.
    }

    // predict...
    {
        const auto f0_val = synthesis_params_array_[params::to_idx(params::params_e::f0)];
        // this alignment prevented runtime error in Eigen
        alignas(16) const std::array<float, params::to_idx(params::params_e::num_params)> inputs {
            synthesis_params_array_[params::to_idx(params::params_e::cc0)],
            synthesis_params_array_[params::to_idx(params::params_e::cc1)],
            synthesis_params_array_[params::to_idx(params::params_e::cc2)],
                                        nvs::pitchLinearToLogScale(f0_val),
            synthesis_params_array_[params::to_idx(params::params_e::voicedness)]};

        model_.forward(&inputs[0]);
    }
    // ...end predict

    scratchBuff.writeAndTransform(model_.getOutputs());
}


void WMS::processBlock (juce::AudioBuffer<float>& outputBuffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    jassert(&outputBuffer);


    // if a completed ScratchBuffer is available, copy it into the non-playing channel.
    if (scratchBuff.tryAcquireForRead()) {
        switchingBuffer.writeToStaleChannel(scratchBuff.getReadPointer());
    }

    for (int samp_idx = 0; samp_idx < block_size_; ++samp_idx) {
        bool hasCrossedOver = false;
        const auto phi = static_cast<float>(
            phasor.tick(hasCrossedOver));

        if (hasCrossedOver) {
            switchingBuffer.promoteStaleToFresh();
        }

        const float* fresh = switchingBuffer.getFreshReadPointer();

        float sample = nvs::cubicInterp(fresh, phi, wavelength);

        sample *= hanning(phi);

        outputBuffer.setSample(0, samp_idx, sample * 0.5f);
        outputBuffer.setSample(1, samp_idx, sample * 0.5f);
    }
}

void WMS::setFrequency(const float newFrequency) {
    // the f0 used for prediction need not be smoothed - we get a discrete waveform anyway
    synthesis_params_array_[params::to_idx(params::params_e::f0)] = newFrequency;
    // BUT there shall be an interpolation over the blocksize for the phasor's frequency!
    /// TODO
    phasor.setFrequency(newFrequency);

}
void WMS::setVoicedness(const float newVoicedness) {
    synthesis_params_array_[params::to_idx(params::params_e::voicedness)] = newVoicedness;
}
void WMS::setCepstralCoefficients(const float cc0, const float cc1, const float cc2) {
    synthesis_params_array_[params::to_idx(params::params_e::cc0)] = cc0;
    synthesis_params_array_[params::to_idx(params::params_e::cc1)] = cc1;
    synthesis_params_array_[params::to_idx(params::params_e::cc2)] = cc2;
}

void WMS::addLogger(juce::FileLogger *logger) {
    logger_ = logger;
}

}