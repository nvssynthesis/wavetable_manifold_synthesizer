#include "PluginProcessor.h"
#include "model_loader.h"
#include "PluginEditor.h"
#include <cmath>
#include "fmt/base.h"
#include "params.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    for (int i = 0; i < params::to_idx(params::params_e::num_params); ++i){
        params::params_e p = params::from_idx(i);
        parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::
                    ParameterID(params::get_param_id(p), 1),
                    params::get_param_name(p),
                    params::get_normalizable_range<float>(p),
                    params::get_default<float>(p)));
    }

    return {parameters.begin(), parameters.end()};
}
//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
fft_(static_cast<int>(std::log2((ModelType::output_size-1) * 2))),
wt_buff_prev_(1, (ModelType::output_size-1) * 2),
wt_buff_curr_(1, (ModelType::output_size-1) * 2),
apvts_(*this, nullptr, juce::Identifier("params"), createParameterLayout()),
logger_(juce::File(nvs::get_designated_plugin_path().getChildFile("log.log")),"Welcome")
{
    auto const modelFilePath =
        "/Users/nicholassolem/development/CLionProjects/wtianns_rtneural/models/gru.json";

    std::ifstream jsonStream(modelFilePath, std::ifstream::binary);
    logger_.logMessage("Loading model from path: " + juce::String(modelFilePath));
    // std::cout << "Loading model from path: " << modelFilePath << std::endl;
    nvs::rtn::loadModel(jsonStream, this->model_);
    logger_.logMessage("Model loaded.");

    juce::File const wav_file = juce::File(nvs::get_designated_plugin_path().getChildFile("debug.wav"));
    bool const written = wav_file.replaceWithData(nullptr, 0);
    jassert(written);
    audio_format_writer_.reset (wav_audio_format_.createWriterFor (new juce::FileOutputStream (wav_file),
                                      48000.0,
                                      wt_buff_curr_.getNumChannels(),
                                      24,
                                      {},
                                      0));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    model_.reset();
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& outputBuffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    auto constexpr f0_max_trained_on = 4000.f;
    auto const f0_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::f0))->load();
    auto const cc0_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc0))->load();
    auto const cc1_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc1))->load();
    auto const cc2_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc2))->load();
    auto const cc3_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc3))->load();
    auto const cc4_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc4))->load();

    const std::vector<float> inputs {cc0_val, cc1_val,  cc2_val, cc3_val,  cc4_val,
                                    -7.7776e-03, -1.0314e-03,  2.4015e-02, -8.4424e-02, -3.4193e-02,
                                     2.1654e-03,  f0_val/f0_max_trained_on};
    std::vector<float> outputs(nvs::rtn::n_output);

    this->model_.forward(&inputs[0]);
    wt_buff_curr_.copyFrom(0, 0, this->model_.getOutputs(), ModelType::output_size);
    if (std::isnan(wt_buff_curr_.getSample(0, 0))) {
        fmt::print("buffer contains NaN");
        logger_.logMessage("buffer contains NaN");
        return;
    }
    fft_.performRealOnlyInverseTransform(wt_buff_curr_.getWritePointer(0));

    int const wavelength = wt_buff_curr_.getNumSamples();

    float mag = wt_buff_curr_.getMagnitude(0, 0, wavelength);
    if (mag == 0.f) {
        mag = 1.f;
    }

    wt_buff_curr_.applyGain(1.f / mag);

    if (audio_format_writer_ != nullptr) {
        if (!wav_written_) {
            audio_format_writer_->writeFromAudioSampleBuffer(wt_buff_curr_, 0, wavelength);
            wav_written_ = true;
        }
    }

    phased_hannings_.allowTransition();

    for (int samp_idx = 0; samp_idx < getBlockSize(); ++samp_idx) {
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

        samp *= 0.07f;
        auto constexpr maxamp = 2.4f;
        samp = samp > maxamp ? maxamp : samp;
        samp = samp < -maxamp ? -maxamp : samp;
        for (int channel = 0; channel < totalNumOutputChannels; ++channel) {

            auto* channelData = outputBuffer.getWritePointer (channel);
            channelData[samp_idx] = samp * 0.9f;
        }
        phased_hannings_.increment_phase(f0_val / getSampleRate());
    }
    wt_buff_prev_ = wt_buff_curr_;
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    // juce::MemoryOutputStream (destData, true).writeFloat (*ap_f0_);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    // *ap_f0_ = juce::MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
