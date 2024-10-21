# VectorModular

![currentModules](https://vectormodular.com/assets/images/curModulesLanding.png)

## About
Inspired by a vision for innovative modular synthesis, **Vector Modular** combines technical precision with dynamic creativity. The project aims to develop high-quality modules that fulfill specific roles within a modular system, while also serving as a learning opportunity for the creator. Committed to pushing the boundaries of sound manipulation, Vector Modular encourages musicians to explore new possibilities in their creative pursuits.


## Contact
Questions? Bugs? Want to get updates?  

Great!  

Please email me at [support@vectormodular.com](mailto:support@vectormodular.com) and I'll try to respond soon.


## Donate
Please consider supporting Vector Modular in developing innovative modules—[donate here](https://vectormodular.com/contribute/).  
**Thank you!**


## Modules
### **baseOsc**
![baseOsc](https://vectormodular.com/assets/images/baseosc.png)

#### Overview

The **baseOsc** is a versatile oscillator for VCV Rack, designed to serve as the core of your sound design setup.

Drawing on robust code from the Mutable Instruments' Braids oscillator, it offers a range of audio output shapes including triangle, sawtooth, pulse (with PWM), sine, and square sub-oscillator.

With its wavetable capability, users can smoothly interpolate between samples and cycles, and control the wavetable index for dynamic sounds.

The baseOsc features unique noise options, including pitched noise and digital clocked noise with modifiable sample cycle length and quantization.

Pitch controls allow for precise adjustments via coarse and fine tuning, octave shifts, and frequency modulation.

FM CV input can be atttenuverted, and the reponse can be toggled between linear and exponential.

When in LFO mode, the module's LED indicators display output polarity, enhancing visual feedback.

The built-in quantizer supports 19 scales and allows you to choose the root note, with LEDs indicating the current scale + root configuration.

A global bit reduction parameter for oscillator output can be set from 1-bit to 16-bit resolution.

The baseOsc is ready to be an essential building block in your patches for shaping both musical and experimental sounds.

#### Features

- Audio output shapes
  - Triangle
  - Sawtooth
  - Pulse with Pulse Width Modulation
  - Sine
  - Square Sub Oscillator
  - Wavetable, Linear, with smooth interpolation between the samples and cycles. Modulation of wavetable index/location.
  - Noise (unaffected by pitch or LFO settings)
  - Pitched noise
  - Digital clocked noise with modulation over sample cycle length and quantization amount
- Pitch controls
  - Coarse tune
  - Fine tune
  - Octave up and down buttons +/- 5 octaves
  - LEDs above coarse knob show the current octave range
  - Frequency Modulation with toggle between linear and exponential
  - FM CV input with attenuverter
  - LFO mode toggle
    - When LFO is active, LEDs above the coarse knob show the polarity of the output in use
  - Quantizer
    - Choose between Off or 19 scales
    - Choose root note
    - Quantizer LEDs light up to indicate current scale+root combination
    - Currently quantized note indicated in these LEDs
- Bit reduction setting
  - Choose between 1-bit to 16-bit oscillator generation resolution.
***  

### **soloMixer**
![soloMixer](https://vectormodular.com/assets/images/solomixer.png)

#### Overview

The **soloMixer** is a versatile mixer for VCV Rack, designed to give you precise control over three input channels—Red, Green, and Blue.  

Each channel features a logarithmic level control and is normalled to the channel below it. If there isn’t a cable plugged into a channel's input, it receives the signal from the channel above, prior to any attenuation. The top channel, R, has a +5V signal by default, which allows the module to be used as a CV offset.  

The module offers visual feedback through color-coded LEDs that indicate attenuation levels and the final mix output. With bipolar and unipolar options, along with soft-clipping on each output, the soloMixer is perfect for shaping sounds.  

The soloMixer also tracks two states of solo configurations.  

This makes it easy to quickly toggle between two sets of output mixes, which lends itself well to performance or quick comparisons.  

The individual outputs on each channel also follow the active solo configuration.

#### Features

- 3 inputs: Red, Green, and Blue channels
- Logarithmic level control for each input
- Inputs normalled to the channel below for easy chaining
- Color-coded LED indicates attenuation level for each channel
- Final mix output can be inverted on the +/- output or maintain original polarity on the + "M" output
- Final mix output LED color based on the sum of the 3 channels
- Soft-clipping overdrive on each output
- +5V on the top channel (Channel R) when nothing is plugged in, allowing for CV offset output. This cascades via the normalization configuration.
- Solo functionality for R, G, and B channels; multiple channels can be active
- Two solo states (1 and 2) toggleable via a button next to the mix knob, with corresponding LEDs for each channel's solo state
***

### **baseTrig**
![baseTrig](https://vectormodular.com/assets/images/basetrig.png)

#### Overview

The **baseTrig** is a powerful clock and trigger generator for VCV Rack, designed to enhance your rhythmic creativity.

With an internal clock generator and the ability to sync to external clock signals, baseTrig offers flexible tempo control for any setup.

Featuring 14 distinct rhythmic interval outputs—including standard notes, triplets, and offbeat options—baseTrig allows you to create intricate patterns that drive drum modules and sequencers

Its intuitive controls, including tap tempo and tempo modulation via CV, enable easy adjustments on the fly. A single red LED blinks on every quarter note, providing a visual cue of the current tempo. 

This module is ideal for a variety of applications, from programming complex drum patterns to generating evolving rhythms in generative music setups and controlling other sequencers in your patch.

#### Features

- Internal clock generator, defined via knob or tap tempo
- Ability to accept an incoming clock signal to define tempo
- Last modified tempo becomes the active tempo
- LED for visually monitoring the current clock signal
- Tempo modulation via CV input, with attenuator
- 14 rhythmic interval outputs
  - 1/16
  - 1/8 Triplet
  - 1/8
  - 1/8 Offbeat
  - 1/4 Triplet
  - 1/4
  - 1/4 Offbeat
  - 1/2
  - 1/2 Offbeat
  - 3/4
  - Whole
  - 5/4
  - 6/4
  - 7/4
- Reset via button, trig or gate input
***

### **3i/9o**
![3i/9o](https://vectormodular.com/assets/images/3i9o.png)

#### Overview

The **3i/9o** is a multiple for VCV Rack that clones signals.  

It features three sets of inputs with normalled connections, allowing for flexible chaining based on your patch.  

You can configure it for 1 input to 9 outputs, 1 input to 6 outputs and 1 input to 3 outputs, or 3 sets of 1 input to 3 outputs. 

An LED indicates the signal polarity at the set's outputs: red for negative, blue for positive.  

Even though it's possible to stack cables in VCV Rack, this module helps with organization and visualization of duplicated signals.

#### Features

- 3 sets of 1 input to 3 outputs
- Inputs 2 and 3 normalled to preceding outputs
- 1 input to 9 outputs, 1 input to 6 outputs and 1 input to 3 outputs, or 3 inputs to 3 outputs
- LED color based on the signal at the set of outputs
- Red for negative, blue for positive
