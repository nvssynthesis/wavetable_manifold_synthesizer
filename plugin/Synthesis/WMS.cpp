#include "WMS.h"

namespace nvs {


//==============================SwitchingBuffer==============================
WMS::SwitchingBuffer::SwitchingBuffer()
    : buffers(numChannels, wavelength)
{}

// the chanel currently being read by synthesis.
[[nodiscard]]
const float* WMS::SwitchingBuffer::getFreshReadPointer() const noexcept {
    return buffers.getReadPointer(freshChannel_);
}

// the channel that is safe to overwrite.
[[nodiscard]]
float* WMS::SwitchingBuffer::getStaleWritePointer() noexcept {
    return buffers.getWritePointer(staleChannel_);
}

// copy a complete waveform into the non-playing channel.
// this does *NOT* make the new waveform active.
void WMS::SwitchingBuffer::writeToStaleChannel(const float* source) noexcept {
    jassert(source != nullptr);

    buffers.copyFrom(
        staleChannel_,
        0,
        source,
        wavelength);
}

// make the previously-stale channel the new playing channel.
// O(1): no sample data is copied.
void WMS::SwitchingBuffer::promoteStaleToFresh() noexcept {
    std::swap(freshChannel_, staleChannel_);
}

[[nodiscard]]
int WMS::SwitchingBuffer::getFreshChannelIndex() const noexcept {
    return freshChannel_;
}

[[nodiscard]]
int WMS::SwitchingBuffer::getStaleChannelIndex() const noexcept {
    return staleChannel_;
}
//==============================ScratchBuffer==============================
WMS::ScratchBuffer::ScratchBuffer()
    : buff(1, wavelength),
      fft_(static_cast<int>(
          std::log2(ModelType::output_size - 1) + 1))
{
    buff.clear();
    jassert(ModelType::output_size <= wavelength);
}

// FOR TIMER THREAD!
// returns true if the ScratchBuffer is available for a new ANN prediction.
[[nodiscard]]
bool WMS::ScratchBuffer::tryBeginWrite() const noexcept {
    // if the consumer hasn't consumed the previous result, leave it alone and don't generate a new prediction.
    return !readyToRead_.load(std::memory_order_acquire);
}

// TIMER THREAD!
// for after tryBeginWrite() succeeds and network has predicted outputs.
void WMS::ScratchBuffer::writeAndTransform(const float* source) {
    jassert(source != nullptr);
    jassert(!readyToRead_.load(std::memory_order_relaxed));

    std::transform(
        source, source + ModelType::output_size,
        buff.getWritePointer(0),
        [](const float x) {
            return std::expm1(std::max(0.0f, x));
        }
    );

    fft_.performRealOnlyInverseTransform(buff.getWritePointer(0));

    // anti-aliasing would go here:
    // antiAlias(f0, fs);

    normalize();

    // publish the finished waveform.
    readyToRead_.store(
        true,
        std::memory_order_release);
}

// FOR AUDIO THREAD!
// claims ready waveform.
// upon success, the producer must not touch buff until this consumer has finished copying.
[[nodiscard]]
bool WMS::ScratchBuffer::tryAcquireForRead() noexcept {
    bool expected = true;

    return readyToRead_.compare_exchange_strong(
        expected,
        false,
        std::memory_order_acquire,
        std::memory_order_relaxed);
}

[[nodiscard]]
const float* WMS::ScratchBuffer::getReadPointer() const noexcept {
    return buff.getReadPointer(0);
}


//=================================WMS=================================
WMS::WMS() {
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
    const auto mfp = modelFilePath.toStdString();
    std::ifstream jsonStream(mfp, std::ifstream::binary);
    if (!jsonStream.is_open()) {
        if (logger_) {
            logger_->logMessage("Model loaded from " + modelFilePath);
        }
        jassertfalse;
        return;
    }
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

    {// predict
        const auto f0_val = synthesis_params_array_[params::to_idx(params::params_e::f0)];
        // this alignment prevented runtime error in Eigen
        alignas(16) const std::array<float, params::to_idx(params::params_e::num_params)> inputs {
            synthesis_params_array_[params::to_idx(params::params_e::cc0)],
            synthesis_params_array_[params::to_idx(params::params_e::cc1)],
            synthesis_params_array_[params::to_idx(params::params_e::cc2)],
            synthesis_params_array_[params::to_idx(params::params_e::cc3)],
            synthesis_params_array_[params::to_idx(params::params_e::cc4)],
            synthesis_params_array_[params::to_idx(params::params_e::cc5)],
            synthesis_params_array_[params::to_idx(params::params_e::cc6)],
            synthesis_params_array_[params::to_idx(params::params_e::cc7)],
                                        nvs::pitchLinearToLogScale(f0_val),
            synthesis_params_array_[params::to_idx(params::params_e::voicedness)]};

        model_.forward(&inputs[0]);
    }

    scratchBuff.writeAndTransform(model_.getOutputs());
}


void WMS::processBlock (juce::AudioBuffer<float>& outputBuffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

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
void WMS::setCepstralCoefficient(const params::params_e cc, const float val){
    synthesis_params_array_[params::to_idx(cc)] = val;
}

void WMS::addLogger(juce::FileLogger *logger) {
    logger_ = logger;
}



}   // nnamespace nvs