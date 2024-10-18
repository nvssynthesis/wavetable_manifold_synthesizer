#include "PluginProcessor.h"
#include "model_loader.h"
#include "PluginEditor.h"
#include <cmath>

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
windowing_function_(ModelType::output_size, juce::dsp::WindowingFunction<float>::WindowingMethod::blackman),
freq_buff_(1, (ModelType::output_size-1) * 2),
logger_(juce::File(get_designated_plugin_path().getChildFile("log.log")),"Welcome")
{
    auto const modelFilePath =
        "/Users/nicholassolem/development/CLionProjects/wtianns_rtneural/models/rt_model_2024-05-28_11-54-32.json";

    std::ifstream jsonStream(modelFilePath, std::ifstream::binary);
    logger_.logMessage("Loading model from path: " + juce::String(modelFilePath));
    // std::cout << "Loading model from path: " << modelFilePath << std::endl;
    loadModel(jsonStream, this->model_);

    juce::File const wav_file = juce::File(get_designated_plugin_path().getChildFile("debug.wav"));
    bool const written = wav_file.replaceWithData(nullptr, 0);
    jassert(written);
    audio_format_writer_.reset (wav_audio_format_.createWriterFor (new juce::FileOutputStream (wav_file),
                                      48000.0,
                                      freq_buff_.getNumChannels(),
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

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    const std::vector<float> inputs {-7.3205e+00, -9.7124e-01,  2.0997e-02, -2.1979e-02,  5.9661e-02,
                                    -7.7776e-03, -1.0314e-03,  2.4015e-02, -8.4424e-02, -3.4193e-02,
                                     7.1654e-02,  2.8041e-03};
    std::vector<float> outputs(n_output);

    this->model_.forward(&inputs[0]);
    freq_buff_.copyFrom(0, 0, this->model_.getOutputs(), ModelType::output_size);

    fft_.performRealOnlyInverseTransform(freq_buff_.getWritePointer(0));

    float mag = freq_buff_.getMagnitude(0, 0, freq_buff_.getNumSamples());
    if (mag == 0.f) {
        mag = 1.f;
    }

    freq_buff_.applyGain(1.f / mag);

    if (audio_format_writer_ != nullptr) {
        if (!wav_written_) {
            audio_format_writer_->writeFromAudioSampleBuffer(freq_buff_, 0, freq_buff_.getNumSamples());
            wav_written_ = true;
        }
    }

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);
        // ..do something to the data...
        for (int samp_idx = 0; samp_idx < getBlockSize(); ++samp_idx) {
            channelData[samp_idx] = freq_buff_.getSample(0, samp_idx);
        }
    }
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
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
