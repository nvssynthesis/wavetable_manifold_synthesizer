#include "PluginProcessor.h"
#include "model_loader.h"
#include "PluginEditor.h"
#include "params/params.h"

AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<RangedAudioParameter>> parameters;

    for (int i = 0; i < params::to_idx(params::params_e::num_params); ++i)
    {
        const params::params_e p = params::from_idx(i);
        parameters.push_back(std::make_unique<AudioParameterFloat>(
                    ParameterID(params::get_param_id(p), 1),
                    params::get_param_name(p),
                    params::get_normalizable_range<float>(p),
                    params::get_default<float>(p)));
    }

    return {parameters.begin(), parameters.end()};
}
//==============================================================================
WMSAudioProcessor::WMSAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
apvts_(*this, nullptr, Identifier("params"), createParameterLayout()),
logger_(File(nvs::get_designated_plugin_path().getChildFile("log.log")),"Welcome")
{
    wms_.addLogger(&logger_);
    wms_.loadModel(nvs::rtn::getModelFilename());
}

WMSAudioProcessor::~WMSAudioProcessor() = default;

//==============================================================================
const String WMSAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WMSAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WMSAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WMSAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WMSAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WMSAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WMSAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WMSAudioProcessor::setCurrentProgram (int index)
{
    ignoreUnused (index);
}

const String WMSAudioProcessor::getProgramName (int index)
{
    ignoreUnused (index);
    return {};
}

void WMSAudioProcessor::changeProgramName (int index, const String& newName)
{
    ignoreUnused (index, newName);
}

//==============================================================================
void WMSAudioProcessor::prepareToPlay (const double sampleRate, const int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    wms_.resetProcessing(sampleRate, samplesPerBlock);
}

void WMSAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool WMSAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

std::unordered_map<int, params::params_e> CCMap {
    {20, params::params_e::cc0},
    {21, params::params_e::cc1},
    {22, params::params_e::cc2},
    {23, params::params_e::cc3},
    {24, params::params_e::cc4},
    {25, params::params_e::cc5},
    {26, params::params_e::cc6},
    {27, params::params_e::cc7},
    {28, params::params_e::f0},
    {29, params::params_e::voicedness}
};

float remap(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float controlChangeToFloat(const params::params_e param, const int val) {

    if (param == params::params_e::f0) {
        const auto midiNoteNum = static_cast<float>(val);
        const float val01 = midiNoteNum / 127.f;
        return val01;
    }
    if (param == params::params_e::voicedness) {
        const float v01 = static_cast<float>(val) / 127.f;
        return v01;
    }
    // then it's cepstral coef
    // 0 maps to -3, 127 maps to +3 (covers 3 standard deviations)
    const float v01 = static_cast<float>(val) / 127.f;
    const float v = v01 * 6.f - 3.f;    // [-3 .. 3]


    // but the param itself has range [-5 to 5]
    const float vRemap = remap(v, -5.f, 5.f, 0.f, 1.f);
    return vRemap;
}

void WMSAudioProcessor::processBlock (AudioBuffer<float>& outputBuffer,
                                              MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    for (const auto metadata : midiMessages) {

        if (const auto msg = metadata.getMessage(); msg.isController()) {
            const int ccNum = msg.getControllerNumber();
            const int ccVal = msg.getControllerValue();

            if (const auto it = CCMap.find(ccNum); it != CCMap.end()) {
                const auto paramIndex = params::to_idx(it->second);
                currentParamNormalizedVals[paramIndex].store(controlChangeToFloat(it->second, ccVal));
            }
        } else if (msg.isNoteOn()) {

        } else if (msg.isNoteOff()) {

        }
    }

    const auto f0_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::f0))->load();
    const auto voiced_val = apvts_.getRawParameterValue(params::get_param_id(params::params_e::voicedness))->load();

    wms_.setFrequency(f0_val);
    wms_.setVoicedness(voiced_val);


    for (int i = 0; i < params::num_cc_coeffs; ++i) {
        const auto cc_param = static_cast<params::params_e>(i + params::cc_offset);
        const auto cc_id = params::get_param_id(cc_param);
        const auto cc_val = apvts_.getRawParameterValue(cc_id)->load();
        wms_.setCepstralCoefficient(cc_param, cc_val);
    }

    wms_.processBlock(outputBuffer, midiMessages);

    if (const auto peak = outputBuffer.getMagnitude(0, 0, outputBuffer.getNumSamples()); peak > 1.f)
    {
        logger_.logMessage("Peaks exceeded limit; clipping output buffer.");
        
        for (auto chan = 0; chan < outputBuffer.getNumChannels(); ++chan)
        {
            const auto writePtr = outputBuffer.getWritePointer(chan);
            
            for (auto samp = 0; samp < outputBuffer.getNumSamples(); ++samp)
            {
                writePtr[samp] = jlimit(-1.f, 1.f, writePtr[samp]);
            }
        }
    }
}

//==============================================================================
bool WMSAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* WMSAudioProcessor::createEditor()
{
    return new WMSProcessorEditor (*this);
}

//==============================================================================
void WMSAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    // MemoryOutputStream (destData, true).writeFloat (*ap_f0_);

    // need to do something about apvts
    // copy state
    // create xml from state
    // add stuff to it if theres special info, new attributes
    // copyXMLtobinary
}

void WMSAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    // *ap_f0_ = MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat();

    // need to do something about apvts

    // get bit of data
    // xml from binary
    // look at params individually, OR pull out apvts from xml
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WMSAudioProcessor();
}
