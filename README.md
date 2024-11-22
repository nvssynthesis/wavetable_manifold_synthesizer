Wavetable Manifold Synthesis plugin in [JUCE](https://github.com/juce-framework/JUCE), implemented using
[RTNeural](https://github.com/jatinchowdhury18/RTNeural).

Wavetable Manifold Synthesis (WMS), formerly called Wavetable-Inspired Artificial Neural Network Synthesis (WTIANNS), 
is a form of sound synthesis developed by [nvssynthesis](https://github.com/nvssynthesis). WMS is closely related to 
wavetable synthesis, except that instead of storing wavetables in memory, they are generated in realtime by a neural 
network. The network for WMS should be trained using timbral features as inputs, and the corresponding magnitude spectra 
as outputs. This means that after successful training, the user can control the timbral content of the wavetables in
realtime based on the timbral features used.

nvssynthesis currently maintains 2 repos for WMS: 
[wms_torch](https://github.com/nvssynthesis/wms_torch) (for training with [PyTorch](https://github.com/pytorch/pytorch)), 
and [wavetable_manifold_synthesizer](https://github.com/nvssynthesis/wavetable_manifold_synthesizer) (a realtime sound
synthesizer plugin). At this moment, the WMS plugin is a minimum working example of the synthesis method, with sliders 
for fundamental frequency (f0) along with the timbral controls. In WMS, f0 also has an effect on timbre because the model
is conditioned on it during training. The plugin does not yet involve MIDI control or plenty of nice features that make
it a full instrument; see the list of 'big TODOs' below. 

The architecture currently used for WMS is a Gated Recurrent Unit (GRU) network, allowing some interesting qualities like 
time dependence and hysteresis of the synthesizer's timbre. You may tweak this if you wish; however, the WMS synthesizer 
plugin statically assumes a particular architecture corresponding to the one outlined in ./params.json. Note that this 
architecture is subject to change, but in general, the two repos will change in a matching manner.

Big TODOs:
-Debug issue of click/glitches at certain wave-transitioning points
-Implement MIDI control
-Provide interface to select from bank of trained networks (including of course custom user models)
-Provide interface & visualization for navigating through the timbre space in several ways, including:
––chaotic attractors 
--following the timbral envelopes of some other sound, possibly in a buffer or from a sidechain input, mimicking the 
quality of a target sound using the sounds that the model was trained on.
