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
apvts_(*this, nullptr, juce::Identifier("params"), createParameterLayout()),
logger_(juce::File(nvs::get_designated_plugin_path().getChildFile("log.log")),"Welcome")
{
    wms_.addLogger(&logger_);
    wms_.loadModel(nvs::rtn::getModelFilename());
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
    wms_.reset(sampleRate, samplesPerBlock);
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto const f0_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::f0))->load();
    auto const voiced_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::voicedness))->load();
    auto const cc0_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc0))->load();
    auto const cc1_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc1))->load();
    auto const cc2_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::cc2))->load();
    wms_.setFrequency(f0_val);
    wms_.setVoicedness(voiced_val);
    wms_.setCepstralCoefficients(cc0_val, cc1_val, cc2_val);
    wms_.processBlock(outputBuffer, midiMessages);

    auto peak = outputBuffer.getMagnitude(0, 0, outputBuffer.getNumSamples());
    if (peak > 1.f) {
        logger_.logMessage("Peaks exceeded limit; clipping output buffer.");
        for (auto chan = 0; chan < outputBuffer.getNumChannels(); ++chan) {
            auto writePtr = outputBuffer.getWritePointer(chan);
            for (auto samp = 0; samp < outputBuffer.getNumSamples(); ++samp) {
                writePtr[samp] = juce::jlimit(-1.f, 1.f, writePtr[samp]);
            }
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
    // juce::MemoryOutputStream (destData, true).writeFloat (*ap_f0_);

    // need to do something about apvts
    // copy state
    // create xml from state
    // add stuff to it if theres special info, new attributes
    // copyXMLtobinary
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    // *ap_f0_ = juce::MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat();

    // need to do something about apvts

    // get bit of data
    // xml from binary
    // look at params individually, OR pull out apvts from xml
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
