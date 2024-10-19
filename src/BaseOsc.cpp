/*
 * This file is part of VectorModular.
 * 
 * VectorModular is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * VectorModular is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "plugin.hpp"
#include <cmath>
#include "braids/macro_oscillator.h"
#include "braids/quantizer.h"
#include "braids/quantizer_scales.h"
#include <string>
#include <vector>


struct BaseOsc : Module {
	enum ParamId {
		QNTSCALE_PARAM,
		OCTUP_PARAM,
		COARSETUNE_PARAM,
		OCTDOWN_PARAM,
		QNTROOT_PARAM,
		LFOMODETOGGLE_PARAM,
		FMLINEXPTOGGLE_PARAM,
		FINETUNE_PARAM,
		FMAMT_PARAM,
		PWMAMT_PARAM,
		INDEXMODAMT_PARAM,
		PULSEWIDTH_PARAM,
		INDEX_PARAM,
		BITS_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
		PWM_INPUT,
		INDEXMOD_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TRI_OUTPUT,
		SAW_OUTPUT,
		PULSE_OUTPUT,
		SINE_OUTPUT,
		SUBSQUARE_OUTPUT,
		WAVETABLE_OUTPUT, //WLIN
		NOISE_OUTPUT,
		PITCHEDNOISE_OUTPUT,
		CLOCKEDNOISE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {

		//frequency / LFO output leds
		N5LED_LIGHT,
		N4LED_LIGHT,
		N3LED_LIGHT,
		N2LED_LIGHT,
		N1LED_LIGHT,
		ZEROLED_LIGHT,
		P1LED_LIGHT,
		P2LED_LIGHT,
		P3LED_LIGHT,
		P4LED_LIGHT,
		P5LED_LIGHT,

		//quantizer setting and current quantized note leds
		ENUMS(QNTLEDCSHARP_LIGHT, 3),
		ENUMS(QNTLEDDSHARP_LIGHT, 3),
		ENUMS(QNTLEDFSHARP_LIGHT, 3),
		ENUMS(QNTLEDGSHARP_LIGHT, 3),
		ENUMS(QNTLEDASHARP_LIGHT, 3),
		ENUMS(QNTLEDC_LIGHT, 3),
		ENUMS(QNTLEDD_LIGHT, 3),
		ENUMS(QNTLEDE_LIGHT, 3),
		ENUMS(QNTLEDF_LIGHT, 3),
		ENUMS(QNTLEDG_LIGHT, 3),
		ENUMS(QNTLEDA_LIGHT, 3),
		ENUMS(QNTLEDB_LIGHT, 3),
	
		LIGHTS_LEN
	};

	//quantizer
	braids::Quantizer quantizer;

	//triangle signal generation
	braids::MacroOscillator triOsc;
	dsp::SampleRateConverter<1> triSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> triOutputBuffer;
	
	//sawtooth signal generation
	braids::MacroOscillator sawOsc;
	dsp::SampleRateConverter<1> sawSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> sawOutputBuffer;

	//pulse signal generation
	braids::MacroOscillator pulseOsc;
	dsp::SampleRateConverter<1> pulseSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> pulseOutputBuffer;

	//sine signal generation
	braids::MacroOscillator sineOsc;
	dsp::SampleRateConverter<1> sineSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> sineOutputBuffer;

	//subSQ signal generation
	braids::MacroOscillator subSQOsc;
	dsp::SampleRateConverter<1> subSQSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> subSQOutputBuffer;

	//WLIN signal generation
	braids::MacroOscillator WLINOsc;
	dsp::SampleRateConverter<1> WLINSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> WLINOutputBuffer;

	//Noise signal generation
	braids::MacroOscillator NoiseOsc;
	dsp::SampleRateConverter<1> NoiseSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> NoiseOutputBuffer;
	
	//pitchedNoise signal generation
	braids::MacroOscillator pitchedNoiseOsc;
	dsp::SampleRateConverter<1> pitchedNoiseSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> pitchedNoiseOutputBuffer;

	//clockedNoise signal generation
	braids::MacroOscillator clockedNoiseOsc;
	dsp::SampleRateConverter<1> clockedNoiseSRC;
	dsp::DoubleRingBuffer<dsp::Frame<1>, 256> clockedNoiseOutputBuffer;
	
	//SchmittTriggers for octave buttons
	dsp::SchmittTrigger octUpButton;
	dsp::SchmittTrigger octDownButton;

	//SchmittTrigger for FM mode
	dsp::SchmittTrigger fmModeButton;

	//SchmittTrigger for LFO mode
	dsp::SchmittTrigger lfoModeButton;


	//supported quantizer scales
	const std::vector<std::string> quantizerScales = {
    "Off",
    "Semitones",
    "Major/Ionian",
    "Dorian",
    "Phrygian",
    "Lydian",
    "Mixolydian",
    "Minor/Aeolian",
    "Locrian",
    "Blues major",
    "Blues minor",
    "Pentatonic major",
    "Pentatonic minor",
    "Folk",
    "Japanese",
    "Gamelan",
    "Gypsy",
    "Arabian",
    "Flamenco",
    "Whole tone"
	};

	


	BaseOsc() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(QNTSCALE_PARAM, 0.0f, quantizerScales.size() - 1, 0.0f, "Quantizer Scale", quantizerScales);
		paramQuantities[QNTSCALE_PARAM]->snapEnabled = true;
		configParam(OCTUP_PARAM, 0.f, 1.f, 0.f, "Octave Up"); 
		configParam(COARSETUNE_PARAM, -5.f, 5.f, 0.f, "Coarse Tune");
		configParam(OCTDOWN_PARAM, 0.f, 1.f, 0.f, "Octave Down"); 
		configSwitch(QNTROOT_PARAM, 0.f, 11.f, 0.f, "Quantizer Root", 
             {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"});
		paramQuantities[QNTROOT_PARAM]->snapEnabled = true;
		configParam(LFOMODETOGGLE_PARAM, 0.f, 1.f, 0.f, "LFO Mode Toggle");
		configParam(FMLINEXPTOGGLE_PARAM, 0.f, 1.f, 0.f, "FM Linear/Exponential Toggle");
		configParam(FINETUNE_PARAM, -1.f, 1.f, 0.f, "Fine Tune"); //this will be by +/- 1 semitone, so in pitch calculation divide by 12
		configParam(FMAMT_PARAM, -0.5f, 0.5f, 0.f, "Frequency Modulation Amount"); //attenuverter behavior for flexibility. 1/2 signal to allow for more precise control. open to making this +/- 1 again based on feedback
		configParam(PWMAMT_PARAM, 0.f, 1.f, 0.f, "Pulse Width Modulation Amount");
		configParam(INDEXMODAMT_PARAM, 0.f, 1.f, 0.f, "Index Modulation Amount");
		configParam(PULSEWIDTH_PARAM, -1.f, 1.f, 0.f, "Pulse Width");
		configParam(INDEX_PARAM, 0.f, 1.f, 0.f, "Index");
		configSwitch(BITS_PARAM, 1.f, 16.f, 16.f, "Bit-Depth",{"1-bit", "2-bit", "3-bit", "4-bit", "5-bit", "6-bit", "7-bit", "8-bit", "9-bit", "10-bit", "11-bit", "12-bit", "13-bit", "14-bit", "15-bit", "16-bit"});
		paramQuantities[BITS_PARAM]->snapEnabled = true;
		configInput(VOCT_INPUT, "Pitch V/oct");
		configInput(FM_INPUT, "Frequency Modulation");
		configInput(PWM_INPUT, "Pulse Width Modulation");
		configInput(INDEXMOD_INPUT, "Index Modulation");
		configOutput(TRI_OUTPUT, "Triangle");
		configOutput(SAW_OUTPUT, "Sawtooth");
		configOutput(PULSE_OUTPUT, "Pulse");
		configOutput(SINE_OUTPUT, "Sine");
		configOutput(SUBSQUARE_OUTPUT, "Sub Square");
		configOutput(WAVETABLE_OUTPUT, "Wavetable Linear");
		configOutput(NOISE_OUTPUT, "Noise");
		configOutput(PITCHEDNOISE_OUTPUT, "Pitched Noise");
		configOutput(CLOCKEDNOISE_OUTPUT, "Digital Noise");

		//quantizer init
		quantizer.Init();
		
		//tri output
		std::memset(&triOsc, 0, sizeof(triOsc));
		triOsc.Init();
		
		//saw output
		std::memset(&sawOsc, 0, sizeof(sawOsc));
		sawOsc.Init();

		//pulse output
		std::memset(&pulseOsc, 0, sizeof(pulseOsc));
		pulseOsc.Init();

		//sine output
		std::memset(&sineOsc, 0, sizeof(sineOsc));
		sineOsc.Init();

		//subSQ output
		std::memset(&subSQOsc, 0, sizeof(subSQOsc));
		subSQOsc.Init();

		//WLIN output
		std::memset(&WLINOsc, 0, sizeof(WLINOsc));
		WLINOsc.Init();

		//Noise output
		std::memset(&NoiseOsc, 0, sizeof(NoiseOsc));
		NoiseOsc.Init();

		//pitchedNoise output
		std::memset(&pitchedNoiseOsc, 0, sizeof(pitchedNoiseOsc));
		pitchedNoiseOsc.Init();

		//clockedNoise output
		std::memset(&clockedNoiseOsc, 0, sizeof(clockedNoiseOsc));
		clockedNoiseOsc.Init();

	}

	// Create an array of lights based on ENUMS
const int keyLights[12] = {
    QNTLEDC_LIGHT,       // C
    QNTLEDCSHARP_LIGHT,  // C#
    QNTLEDD_LIGHT,       // D
    QNTLEDDSHARP_LIGHT,  // D#
    QNTLEDE_LIGHT,       // E
    QNTLEDF_LIGHT,       // F
    QNTLEDFSHARP_LIGHT,  // F#
    QNTLEDG_LIGHT,       // G
    QNTLEDGSHARP_LIGHT,  // G#
    QNTLEDA_LIGHT,       // A
    QNTLEDASHARP_LIGHT,  // A#
    QNTLEDB_LIGHT        // B
};

void updateLights(int quantizedNote, const braids::Scale& scale, int root) {
   
    // Determine which notes are active in the scale
    bool active_notes[12] = { false }; // 12 notes: C, C#, D, D#, E, F, F#, G, G#, A, A#, B

     // Calculate the root index (ensure it is within the range of 0 to 11)
    int rootIndex = root % 12;

	// Set the active notes based on the scale
    for (size_t i = 0; i < scale.num_notes; ++i) {
        int note = ((scale.notes[i] / 128) + rootIndex) % 12; // Adjust for root
        active_notes[note] = true; // Mark this note as active
    }

	// Array for black key positions
    const int blackKeys[] = { 1, 3, 6, 8, 10 }; // MIDI numbers for C#, D#, F#, G#, A#
	//light up the keys
    for (int i = 0; i < 12; ++i) {
        if (active_notes[i]) {
            bool isBlackKey = false;
            // Check if the note is a black key
            for (int j = 0; j < sizeof(blackKeys) / sizeof(blackKeys[0]); ++j) {
                if (i == blackKeys[j]) {
                    isBlackKey = true;
                    break;
                }
            }

            if (isBlackKey) {
                // Black keys (C#, D#, F#, G#, A#)
                lights[keyLights[i] + 0].setBrightness(10.0f); // Full yellow (red + green)
                lights[keyLights[i] + 1].setBrightness(10.0f); // Full yellow (red + green)
                lights[keyLights[i] + 2].setBrightness(0.0f);
            } else {
                // White keys (C, D, E, F, G, A, B)
                lights[keyLights[i] + 0].setBrightness(10.0f); // Full white (red + green + blue)
                lights[keyLights[i] + 1].setBrightness(10.0f); // Full white (red + green + blue)
                lights[keyLights[i] + 2].setBrightness(10.0f); // Full white (red + green + blue)
            }
        }else if(!active_notes[i]){
			lights[keyLights[i]+0].setBrightness(0.0f); // off
			lights[keyLights[i]+1].setBrightness(0.0f); 
			lights[keyLights[i]+2].setBrightness(0.0f); 
		}
	} 

    // Highlight the quantized note
    int quantizedNoteIndex = quantizedNote % 12; // Get the index of the quantized note
    lights[keyLights[quantizedNoteIndex]+0].setBrightness(0.0f);
	lights[keyLights[quantizedNoteIndex]+1].setBrightness(10.0f); // Set brightness to full green
	lights[keyLights[quantizedNoteIndex]+2].setBrightness(0.0f);
}

	//state variables. these need to be saved via json
	bool isLFOmode = false;
	bool isLINfm = true;
	int octOffsetButtons = 0; //range of -5 to +5

	//pitch from CV inputs
	float sumPitchCV = 0.f; //calculated based on pitch inputs
	int32_t pitchBraids = 0;
	float lastPitchLEDcv = 0.1f; //only check pitch LEDs when there is a new pitch cv
	float lfoModeSkipCounter = 0.f;
	float lfoModeSkipThreshold = 0.f; //use this to keep LFO consistent over different sample rates
	bool lastLFOmode = true;

	//quantizer variables
	int quantizerScale = 0;
	int quantizerRoot = 0;

	//pw variables
	float basedPulseWidth = 0.f; //center point
	float pulseWidth = 0.f;

	float basedClockedNoiseCycleLength = 0.f;
	float clockedNoiseCycleLength = 0.f;

	//index variables
	float basedIndex = 0.f; //this is the center point / starting index before modulation
	float wavetableIndex = 0.f;
	
	float basedClockedQuantBits = 0.f;
	float clockedQuantBits = 0.f;

	//oscillator lights, either pitch for regular mode or cv out for lfo mode
	float posNegLEDvalue = 0.f;
	float numConnected = 0.001f;

	//bit setting
	int outputBits = 16;
	int16_t bitMask = 0;


	// save/load variables that aren't based on knobs etc.
	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		
		// Save state variables
		json_object_set_new(rootJ, "isLFOmode", json_boolean(isLFOmode));
		json_object_set_new(rootJ, "isLINfm", json_boolean(isLINfm));
		json_object_set_new(rootJ, "octOffsetButtons", json_integer(octOffsetButtons));
		
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* isLFOmodeJ = json_object_get(rootJ, "isLFOmode");
		if (isLFOmodeJ)
			isLFOmode = json_is_true(isLFOmodeJ);
			
		json_t* isLINfmJ = json_object_get(rootJ, "isLINfm");
		if (isLINfmJ)
			isLINfm = json_is_true(isLINfmJ);
			
		json_t* octOffsetButtonsJ = json_object_get(rootJ, "octOffsetButtons");
		if (octOffsetButtonsJ)
			octOffsetButtons = json_integer_value(octOffsetButtonsJ);
	}


	//main process
	void process(const ProcessArgs& args) override {

	//octave buttons
	if(octUpButton.process(params[OCTUP_PARAM].getValue())){
		if(octOffsetButtons < 5){
			octOffsetButtons++;
		}
	}

	if(octDownButton.process(params[OCTDOWN_PARAM].getValue())){
		if(octOffsetButtons > -5){
			octOffsetButtons--;
		}
	}

	//toggle FM Mode
	if(fmModeButton.process(params[FMLINEXPTOGGLE_PARAM].getValue())){
		isLINfm = !isLINfm; //shorthand for inverting bool values in c++
	}
	
	//toggle LFO Mode
	if(lfoModeButton.process(params[LFOMODETOGGLE_PARAM].getValue())){
		isLFOmode = !isLFOmode;
	}
	

	//calculate pitch cv, allowing for quantization before FM modulation
	quantizerScale = params[QNTSCALE_PARAM].getValue();
	quantizerRoot = (params[QNTROOT_PARAM].getValue()+60)*128;

	sumPitchCV = (inputs[VOCT_INPUT].getVoltage()+params[COARSETUNE_PARAM].getValue() + octOffsetButtons + (params[FINETUNE_PARAM].getValue()/12)); //before fm mod applied

	sumPitchCV = clamp(sumPitchCV,-5.f,5.f);
	
	if(quantizerScale != 0){

		quantizer.Configure(braids::scales[quantizerScale]);

		pitchBraids = (sumPitchCV * 12.0 + 60) * 128;
		
		pitchBraids = quantizer.Process(pitchBraids, quantizerRoot); //quantizer takes these values not the cv range.

		sumPitchCV = ((pitchBraids / 128.0f) - 60.0f) / 12.0f; //get back to cv so fm mod can be applied

		// Determine the quantized note based on the processed pitch
		int quantizedNote = static_cast<int>(pitchBraids / 128.0f) % 12; // Get the note number in the range 0-11
		int quantizedRootLED = static_cast<int>(quantizerRoot / 128.0f) % 12; // Get the note number in the range 0-11
		const braids::Scale& currentScale = braids::scales[quantizerScale];

		// Update the lights based on the current scale and root
		updateLights(quantizedNote,currentScale, quantizedRootLED);
	}else{
		 // all lights are off when the quantizer is off
		for (int i = 0; i < 12; ++i) {
			lights[keyLights[i]+0].setBrightness(0.0f); // off
			lights[keyLights[i]+1].setBrightness(0.0f); 
			lights[keyLights[i]+2].setBrightness(0.0f); 
		}
	}

	//add FM modulation based on active mode
	if(isLINfm){ 
		sumPitchCV = sumPitchCV+(params[FMAMT_PARAM].getValue()*inputs[FM_INPUT].getVoltage());
	}else{
		sumPitchCV = sumPitchCV+(std::pow(2.0f, params[FMAMT_PARAM].getValue() * inputs[FM_INPUT].getVoltage()) - 1.0f);
	}	
	
	pitchBraids = (sumPitchCV * 12.0 + 60) * 128;

	pitchBraids = clamp(pitchBraids, 0, 16383);	

	//pitch led light logic. very clunky, look into optimizing this
	if(lastPitchLEDcv != sumPitchCV || isLFOmode || isLFOmode != lastLFOmode){ 
		
		if(isLFOmode){

			numConnected = 0.001f;
			if(outputs[TRI_OUTPUT].isConnected()){
				numConnected++;
			}
			if(outputs[SAW_OUTPUT].isConnected()){
				numConnected++;
			}
			if(outputs[PULSE_OUTPUT].isConnected()){
				numConnected++;
			}
			if(outputs[SINE_OUTPUT].isConnected()){
				numConnected++;
			}
			if(outputs[SUBSQUARE_OUTPUT].isConnected()){
				numConnected++;
			}
			if(outputs[WAVETABLE_OUTPUT].isConnected()){
				numConnected++;
			}

			numConnected = clamp(numConnected, 0.001f, 10.f); //lookout for divide by zero errors!
			
			if(numConnected > 0.001 && numConnected < 1.9){
			posNegLEDvalue = ((outputs[TRI_OUTPUT].getVoltage()+outputs[SAW_OUTPUT].getVoltage()+outputs[PULSE_OUTPUT].getVoltage()+outputs[SINE_OUTPUT].getVoltage()+outputs[SUBSQUARE_OUTPUT].getVoltage()+outputs[WAVETABLE_OUTPUT].getVoltage())/(numConnected+.001f))*1.1;
			}else{
				if(outputs[SINE_OUTPUT].isConnected()){
					posNegLEDvalue = outputs[SINE_OUTPUT].getVoltage()*1.1;	
				}else if(outputs[TRI_OUTPUT].isConnected()){
					posNegLEDvalue = outputs[TRI_OUTPUT].getVoltage()*1.1;
				}else if(outputs[SAW_OUTPUT].isConnected()){
					posNegLEDvalue = outputs[SAW_OUTPUT].getVoltage()*1.1;
				}else if(outputs[PULSE_OUTPUT].isConnected()){
					posNegLEDvalue = outputs[PULSE_OUTPUT].getVoltage()*1.1;
				}else if(outputs[SUBSQUARE_OUTPUT].isConnected()){
					posNegLEDvalue = outputs[SUBSQUARE_OUTPUT].getVoltage()*1.1;
				}else if(outputs[WAVETABLE_OUTPUT].isConnected()){
					posNegLEDvalue = outputs[WAVETABLE_OUTPUT].getVoltage()*1.1;
				}else{
					posNegLEDvalue = 0;
				}
			}

			lastLFOmode = isLFOmode;
		}else{
			posNegLEDvalue = sumPitchCV;
		}

		if(posNegLEDvalue <= -5){
		lights[N5LED_LIGHT].setBrightness(1.0f);
		lights[N4LED_LIGHT].setBrightness(1.0f);
		lights[N3LED_LIGHT].setBrightness(1.0f);
		lights[N2LED_LIGHT].setBrightness(1.0f);
		lights[N1LED_LIGHT].setBrightness(1.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(0.0f);
		lights[P2LED_LIGHT].setBrightness(0.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue > -5 && posNegLEDvalue <= -4){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(1.0f);
		lights[N3LED_LIGHT].setBrightness(1.0f);
		lights[N2LED_LIGHT].setBrightness(1.0f);
		lights[N1LED_LIGHT].setBrightness(1.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(0.0f);
		lights[P2LED_LIGHT].setBrightness(0.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue > -4 && posNegLEDvalue <= -3){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(1.0f);
		lights[N2LED_LIGHT].setBrightness(1.0f);
		lights[N1LED_LIGHT].setBrightness(1.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(0.0f);
		lights[P2LED_LIGHT].setBrightness(0.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue > -3 && posNegLEDvalue <= -2){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(1.0f);
		lights[N1LED_LIGHT].setBrightness(1.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(0.0f);
		lights[P2LED_LIGHT].setBrightness(0.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue > -2 && posNegLEDvalue <= -1){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(0.0f);
		lights[N1LED_LIGHT].setBrightness(1.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(0.0f);
		lights[P2LED_LIGHT].setBrightness(0.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue > -1 && posNegLEDvalue < 1){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(0.0f);
		lights[N1LED_LIGHT].setBrightness(0.0f);
		lights[ZEROLED_LIGHT].setBrightness(1.0f);
		lights[P1LED_LIGHT].setBrightness(0.0f);
		lights[P2LED_LIGHT].setBrightness(0.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue >= 1 && posNegLEDvalue < 2){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(0.0f);
		lights[N1LED_LIGHT].setBrightness(0.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(1.0f);
		lights[P2LED_LIGHT].setBrightness(0.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue >= 2 && posNegLEDvalue < 3){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(0.0f);
		lights[N1LED_LIGHT].setBrightness(0.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(1.0f);
		lights[P2LED_LIGHT].setBrightness(1.0f);
		lights[P3LED_LIGHT].setBrightness(0.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue >= 3 && posNegLEDvalue < 4){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(0.0f);
		lights[N1LED_LIGHT].setBrightness(0.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(1.0f);
		lights[P2LED_LIGHT].setBrightness(1.0f);
		lights[P3LED_LIGHT].setBrightness(1.0f);
		lights[P4LED_LIGHT].setBrightness(0.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue >= 4 && posNegLEDvalue < 5){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(0.0f);
		lights[N1LED_LIGHT].setBrightness(0.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(1.0f);
		lights[P2LED_LIGHT].setBrightness(1.0f);
		lights[P3LED_LIGHT].setBrightness(1.0f);
		lights[P4LED_LIGHT].setBrightness(1.0f);
		lights[P5LED_LIGHT].setBrightness(0.0f);
		}
		if(posNegLEDvalue >= 5){
		lights[N5LED_LIGHT].setBrightness(0.0f);
		lights[N4LED_LIGHT].setBrightness(0.0f);
		lights[N3LED_LIGHT].setBrightness(0.0f);
		lights[N2LED_LIGHT].setBrightness(0.0f);
		lights[N1LED_LIGHT].setBrightness(0.0f);
		lights[ZEROLED_LIGHT].setBrightness(0.0f);
		lights[P1LED_LIGHT].setBrightness(1.0f);
		lights[P2LED_LIGHT].setBrightness(1.0f);
		lights[P3LED_LIGHT].setBrightness(1.0f);
		lights[P4LED_LIGHT].setBrightness(1.0f);
		lights[P5LED_LIGHT].setBrightness(1.0f);
		}

	} //end of LED statements


	lastPitchLEDcv = sumPitchCV; //this goes after the LEDs are processed. if they are equal the next process, the led logic will not be looked at.

	//get the pulse width value

	basedPulseWidth = 32000 * (std::abs(params[PULSEWIDTH_PARAM].getValue()));

	if(inputs[PWM_INPUT].getVoltage() == 0 || !inputs[PWM_INPUT].isConnected() || params[PWMAMT_PARAM].getValue() == 0){
		pulseWidth = basedPulseWidth;
	} else{
		pulseWidth = basedPulseWidth + ((std::abs(inputs[PWM_INPUT].getVoltage() / 5.0f)) * params[PWMAMT_PARAM].getValue() * 32000);
	}

	pulseWidth = clamp(pulseWidth, 0.f, 32000.0f);

	// Get the wavetable index value. this should allow smooth modulation with incoming negative voltage even when index is 0

	basedIndex = 32767 * params[INDEX_PARAM].getValue();

	if (inputs[INDEXMOD_INPUT].getVoltage() == 0 || !inputs[INDEXMOD_INPUT].isConnected() || params[INDEXMODAMT_PARAM].getValue() == 0) {
		wavetableIndex = basedIndex;
	} else {
		
		wavetableIndex = basedIndex + ((params[INDEXMODAMT_PARAM].getValue()/15.0f)*inputs[INDEXMOD_INPUT].getVoltage()*32767);
		
	}

	// Ensure wavetableIndex is within bounds using modulo
	wavetableIndex = fmod(wavetableIndex, 65534); // 65534 = 2 * 32767

	// Handle negative values
	if (wavetableIndex < 0) {
		wavetableIndex = -wavetableIndex; // Reflect negative values
	}

	// Apply final bouncing logic
	if (wavetableIndex > 32767) {
		wavetableIndex = 65534 - wavetableIndex; // Reflect back into the range
	}

	wavetableIndex = clamp(wavetableIndex, 0.f, 32767.0f);


	//use pw and index inputs and controls to interact with the clocked noise settings. pw goes to cycle length, index goes to quantize bits amount

	basedClockedNoiseCycleLength = 32767 * (std::abs(params[PULSEWIDTH_PARAM].getValue()));

	if (inputs[PWM_INPUT].getVoltage() == 0 || !inputs[PWM_INPUT].isConnected() || params[PWMAMT_PARAM].getValue() == 0) {
		clockedNoiseCycleLength = basedClockedNoiseCycleLength;
	} else {
		clockedNoiseCycleLength = basedClockedNoiseCycleLength + ((params[PWMAMT_PARAM].getValue()/10.0f)*inputs[PWM_INPUT].getVoltage()*32767);
	}

	clockedNoiseCycleLength = fmod(clockedNoiseCycleLength, 65534);

	if (clockedNoiseCycleLength < 0) {
		clockedNoiseCycleLength = -clockedNoiseCycleLength; 
	}

	if (clockedNoiseCycleLength > 32767) {
		clockedNoiseCycleLength = 65534 - clockedNoiseCycleLength; 
	}

	clockedNoiseCycleLength = clamp(clockedNoiseCycleLength, 0.f, 32767.0f);

	basedClockedQuantBits = 32767 * params[INDEX_PARAM].getValue();

	if (inputs[INDEXMOD_INPUT].getVoltage() == 0 || !inputs[INDEXMOD_INPUT].isConnected() || params[INDEXMODAMT_PARAM].getValue() == 0) {
		clockedQuantBits = basedClockedQuantBits;
	} else {
		clockedQuantBits = basedClockedQuantBits + ((params[INDEXMODAMT_PARAM].getValue()/10.0f)*inputs[INDEXMOD_INPUT].getVoltage()*32767);
	}

	clockedQuantBits = fmod(clockedQuantBits, 65534);

	if (clockedQuantBits < 0) {
		clockedQuantBits = -clockedQuantBits; 
	}

	if (clockedQuantBits > 32767) {
		clockedQuantBits = 65534 - clockedQuantBits; 
	}

	clockedQuantBits = clamp(clockedQuantBits, 0.f, 32767.0f);

	//bit rate
	outputBits = params[BITS_PARAM].getValue();
	bitMask = (1 << outputBits) - 1;
	bitMask = bitMask << (16 - outputBits); 		



	//lfo mode handled by using if statement to decide if outputs should render

	if(lfoModeSkipCounter == 0 || !isLFOmode){

	//tri output
	if(outputs[TRI_OUTPUT].isConnected()){

		if(triOutputBuffer.empty()){

			triOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_MORPH);
			triOsc.set_parameters(0,0); //param1 for the oscillator mix. param2 is lp so 0 is full open

		

		triOsc.set_pitch(pitchBraids);

		uint8_t triSync_buffer[24] = {};

		int16_t triRender_buffer[24];
		triOsc.Render(triSync_buffer, triRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			triRender_buffer[i] &= bitMask; // Apply the generated mask to each sample
		}

		// Sample rate convert
		dsp::Frame<1> triIn[24];
		for (int i = 0; i < 24; i++) {
			triIn[i].samples[0] = triRender_buffer[i] / 32768.0;
		}
		
		
		
		triSRC.setRates(96000, args.sampleRate);

		int triInLen = 24;
		int triOutLen = triOutputBuffer.capacity();
		triSRC.process(triIn, &triInLen, triOutputBuffer.endData(), &triOutLen);
		triOutputBuffer.endIncr(triOutLen);
		}

		// Output triangle
		if (!triOutputBuffer.empty()) {
			dsp::Frame<1> triF = triOutputBuffer.shift();
			outputs[TRI_OUTPUT].setVoltage(5.0 * triF.samples[0]);
		}

	}


	//saw output
	if(outputs[SAW_OUTPUT].isConnected()){

		if(sawOutputBuffer.empty()){

			sawOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_MORPH);
			sawOsc.set_parameters(10923,0);

		//pitch snippet was here if it stops working

		sawOsc.set_pitch(pitchBraids);

		uint8_t sawSync_buffer[24] = {};

		int16_t sawRender_buffer[24];
		sawOsc.Render(sawSync_buffer, sawRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			sawRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> sawIn[24];
		for (int i = 0; i < 24; i++) {
			sawIn[i].samples[0] = sawRender_buffer[i] / 32768.0;
		}
		sawSRC.setRates(96000, args.sampleRate);

		int sawInLen = 24;
		int sawOutLen = sawOutputBuffer.capacity();
		sawSRC.process(sawIn, &sawInLen, sawOutputBuffer.endData(), &sawOutLen);
		sawOutputBuffer.endIncr(sawOutLen);
		}

		// Output saw
		if (!sawOutputBuffer.empty()) {
			dsp::Frame<1> sawF = sawOutputBuffer.shift();
			outputs[SAW_OUTPUT].setVoltage(5.0 * sawF.samples[0]);
		}

	}

	//pulse output
	if(outputs[PULSE_OUTPUT].isConnected()){

		if(pulseOutputBuffer.empty()){

			pulseOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_SAW_SQUARE);
			pulseOsc.set_parameters(pulseWidth,32767); //param1 for the phase. param2 is for the osc shape

		pulseOsc.set_pitch(pitchBraids);

		uint8_t pulseSync_buffer[24] = {};

		int16_t pulseRender_buffer[24];
		pulseOsc.Render(pulseSync_buffer, pulseRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			pulseRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> pulseIn[24];
		for (int i = 0; i < 24; i++) {
			pulseIn[i].samples[0] = pulseRender_buffer[i] / 32768.0;
		}
		pulseSRC.setRates(96000, args.sampleRate);

		int pulseInLen = 24;
		int pulseOutLen = pulseOutputBuffer.capacity();
		pulseSRC.process(pulseIn, &pulseInLen, pulseOutputBuffer.endData(), &pulseOutLen);
		pulseOutputBuffer.endIncr(pulseOutLen);
		}

		// Output pulse
		if (!pulseOutputBuffer.empty()) {
			dsp::Frame<1> pulseF = pulseOutputBuffer.shift();
			outputs[PULSE_OUTPUT].setVoltage(5.0 * pulseF.samples[0]);
		}

	}

	//sine output. also the basis of the lfo lights if nothing is connected
	if(outputs[SINE_OUTPUT].isConnected()){

		if(sineOutputBuffer.empty()){

			sineOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_HARMONICS);
			sineOsc.set_parameters(0,0);
			sineOsc.set_pitch(pitchBraids);

		uint8_t sineSync_buffer[24] = {};

		int16_t sineRender_buffer[24];
		sineOsc.Render(sineSync_buffer, sineRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			sineRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> sineIn[24];
		for (int i = 0; i < 24; i++) {
			sineIn[i].samples[0] = sineRender_buffer[i] / 32768.0;
		}
		sineSRC.setRates(96000, args.sampleRate);

		int sineInLen = 24;
		int sineOutLen = sineOutputBuffer.capacity();
		sineSRC.process(sineIn, &sineInLen, sineOutputBuffer.endData(), &sineOutLen);
		sineOutputBuffer.endIncr(sineOutLen);
		}

		// Output sine
		if (!sineOutputBuffer.empty()) {
			dsp::Frame<1> sineF = sineOutputBuffer.shift();
			outputs[SINE_OUTPUT].setVoltage(5.0 * sineF.samples[0]);
		}

	}

	//subSQ output
	if(outputs[SUBSQUARE_OUTPUT].isConnected()){

		if(subSQOutputBuffer.empty()){

			subSQOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_SAW_SQUARE);
			subSQOsc.set_parameters(0,32767);
		subSQOsc.set_pitch((sumPitchCV * 12.0 + 48) * 128); //just adjusting down two octaves

		uint8_t subSQSync_buffer[24] = {};

		int16_t subSQRender_buffer[24];
		subSQOsc.Render(subSQSync_buffer, subSQRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			subSQRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> subSQIn[24];
		for (int i = 0; i < 24; i++) {
			subSQIn[i].samples[0] = subSQRender_buffer[i] / 32768.0;
		}
		subSQSRC.setRates(96000, args.sampleRate);

		int subSQInLen = 24;
		int subSQOutLen = subSQOutputBuffer.capacity();
		subSQSRC.process(subSQIn, &subSQInLen, subSQOutputBuffer.endData(), &subSQOutLen);
		subSQOutputBuffer.endIncr(subSQOutLen);
		}

		// Output subSQ
		if (!subSQOutputBuffer.empty()) {
			dsp::Frame<1> subSQF = subSQOutputBuffer.shift();
			outputs[SUBSQUARE_OUTPUT].setVoltage(5.0 * subSQF.samples[0]);
		}

	}

	//WLIN output
	if(outputs[WAVETABLE_OUTPUT].isConnected()){

		if(WLINOutputBuffer.empty()){

			WLINOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_WAVE_LINE);
			WLINOsc.set_parameters(wavetableIndex,24575); //param 1 gives the index location, param 2 defines the interpolation method. 24575 gives a clean blend/interpolation of waves and samples.
			WLINOsc.set_pitch(pitchBraids);

		uint8_t WLINSync_buffer[24] = {};

		int16_t WLINRender_buffer[24];
		WLINOsc.Render(WLINSync_buffer, WLINRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			WLINRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> WLINIn[24];
		for (int i = 0; i < 24; i++) {
			WLINIn[i].samples[0] = WLINRender_buffer[i] / 32768.0;
		}
		WLINSRC.setRates(96000, args.sampleRate);

		int WLINInLen = 24;
		int WLINOutLen = WLINOutputBuffer.capacity();
		WLINSRC.process(WLINIn, &WLINInLen, WLINOutputBuffer.endData(), &WLINOutLen);
		WLINOutputBuffer.endIncr(WLINOutLen);
		}

		// Output WLIN
		if (!WLINOutputBuffer.empty()) {
			dsp::Frame<1> WLINF = WLINOutputBuffer.shift();
			outputs[WAVETABLE_OUTPUT].setVoltage(5.0 * WLINF.samples[0]);
		}

	}		

	//pitchedNoise output
	if(outputs[PITCHEDNOISE_OUTPUT].isConnected()){

		if(pitchedNoiseOutputBuffer.empty()){

			pitchedNoiseOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_FILTERED_NOISE);
			pitchedNoiseOsc.set_parameters(16385,16385); //param1 for the resonance. param2 is for the filter mode

		

		pitchedNoiseOsc.set_pitch(pitchBraids);

		uint8_t pitchedNoiseSync_buffer[24] = {};

		int16_t pitchedNoiseRender_buffer[24];
		pitchedNoiseOsc.Render(pitchedNoiseSync_buffer, pitchedNoiseRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			pitchedNoiseRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> pitchedNoiseIn[24];
		for (int i = 0; i < 24; i++) {
			pitchedNoiseIn[i].samples[0] = pitchedNoiseRender_buffer[i] / 32768.0;
		}
		pitchedNoiseSRC.setRates(96000, args.sampleRate);

		int pitchedNoiseInLen = 24;
		int pitchedNoiseOutLen = pitchedNoiseOutputBuffer.capacity();
		pitchedNoiseSRC.process(pitchedNoiseIn, &pitchedNoiseInLen, pitchedNoiseOutputBuffer.endData(), &pitchedNoiseOutLen);
		pitchedNoiseOutputBuffer.endIncr(pitchedNoiseOutLen);
		}

		// Output pitchedNoise
		if (!pitchedNoiseOutputBuffer.empty()) {
			dsp::Frame<1> pitchedNoiseF = pitchedNoiseOutputBuffer.shift();
			outputs[PITCHEDNOISE_OUTPUT].setVoltage(5.0 * pitchedNoiseF.samples[0]);
		}

	}

	//clocked noise output
	if(outputs[CLOCKEDNOISE_OUTPUT].isConnected()){

		if(clockedNoiseOutputBuffer.empty()){

			clockedNoiseOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_CLOCKED_NOISE);
			clockedNoiseOsc.set_parameters(clockedNoiseCycleLength,clockedQuantBits); //param1 for the cycle length. param2 is for the quantized bits

		

		clockedNoiseOsc.set_pitch(pitchBraids);

		uint8_t clockedNoiseSync_buffer[24] = {};

		int16_t clockedNoiseRender_buffer[24];
		clockedNoiseOsc.Render(clockedNoiseSync_buffer, clockedNoiseRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			clockedNoiseRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> clockedNoiseIn[24];
		for (int i = 0; i < 24; i++) {
			clockedNoiseIn[i].samples[0] = clockedNoiseRender_buffer[i] / 32768.0;
		}
		clockedNoiseSRC.setRates(96000, args.sampleRate);

		int clockedNoiseInLen = 24;
		int clockedNoiseOutLen = clockedNoiseOutputBuffer.capacity();
		clockedNoiseSRC.process(clockedNoiseIn, &clockedNoiseInLen, clockedNoiseOutputBuffer.endData(), &clockedNoiseOutLen);
		clockedNoiseOutputBuffer.endIncr(clockedNoiseOutLen);
		}

		// Output clockedNoise
		if (!clockedNoiseOutputBuffer.empty()) {
			dsp::Frame<1> clockedNoiseF = clockedNoiseOutputBuffer.shift();
			outputs[CLOCKEDNOISE_OUTPUT].setVoltage(5.0 * clockedNoiseF.samples[0]);
		}

	}
	
	} //encloses the outputs affected by LFO mode

	lfoModeSkipThreshold = args.sampleRate / 48000.0f * 100.0f; // scales threshold based on sample rate. confirmed working pretty okayish
	lfoModeSkipCounter++;

	if(lfoModeSkipCounter > lfoModeSkipThreshold || !isLFOmode){ // makes the output only 1/100th of regular mode in 48khz. then this ratio is maintained for other sample rates. sounds like an ok range, maybe this could be tweaked one day. good for now.
		lfoModeSkipCounter = 0;
	}

	
	// Noise output. Put this after the lfo mode so it doesn't affect it
	if(outputs[NOISE_OUTPUT].isConnected()){

		if(NoiseOutputBuffer.empty()){

			NoiseOsc.set_shape((braids::MacroOscillatorShape) braids::MACRO_OSC_SHAPE_FILTERED_NOISE);
			NoiseOsc.set_parameters(0,26216); //param1 for the resonance. param2 is for the filter mode

		

		NoiseOsc.set_pitch(7680);

		uint8_t NoiseSync_buffer[24] = {};

		int16_t NoiseRender_buffer[24];
		NoiseOsc.Render(NoiseSync_buffer, NoiseRender_buffer, 24);
		
		// Apply bit reduction by masking the lower bits
		for (int i = 0; i < 24; i++) {
			NoiseRender_buffer[i] &= bitMask;
		}
		
		// Sample rate convert

		dsp::Frame<1> NoiseIn[24];
		for (int i = 0; i < 24; i++) {
			NoiseIn[i].samples[0] = NoiseRender_buffer[i] / 32768.0;
		}
		NoiseSRC.setRates(96000, args.sampleRate);

		int NoiseInLen = 24;
		int NoiseOutLen = NoiseOutputBuffer.capacity();
		NoiseSRC.process(NoiseIn, &NoiseInLen, NoiseOutputBuffer.endData(), &NoiseOutLen);
		NoiseOutputBuffer.endIncr(NoiseOutLen);
		}

		// Output Noise
		if (!NoiseOutputBuffer.empty()) {
			dsp::Frame<1> NoiseF = NoiseOutputBuffer.shift();
			outputs[NOISE_OUTPUT].setVoltage(5.0 * NoiseF.samples[0]);
		}

	}
	
	} //don't delete this. end of process

};


struct BaseOscWidget : ModuleWidget {
	BaseOscWidget(BaseOsc* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/BaseOsc.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(53.361, 23.428)), module, BaseOsc::QNTSCALE_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(9.047, 24.138)), module, BaseOsc::OCTUP_PARAM));
		addParam(createParamCentered<Rogan3PSWhite>(mm2px(Vec(30.48, 27.578)), module, BaseOsc::COARSETUNE_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(9.047, 31.018)), module, BaseOsc::OCTDOWN_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(45.062, 31.728)), module, BaseOsc::QNTROOT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(9.047, 41.823)), module, BaseOsc::LFOMODETOGGLE_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(23.336, 41.823)), module, BaseOsc::FMLINEXPTOGGLE_PARAM));
		addParam(createParamCentered<Rogan1PWhite>(mm2px(Vec(9.047, 52.105)), module, BaseOsc::FINETUNE_PARAM));
		addParam(createParamCentered<Rogan1PWhite>(mm2px(Vec(23.336, 52.105)), module, BaseOsc::FMAMT_PARAM));
		addParam(createParamCentered<Rogan1PWhite>(mm2px(Vec(37.624, 52.105)), module, BaseOsc::PWMAMT_PARAM));
		addParam(createParamCentered<Rogan1PWhite>(mm2px(Vec(51.913, 52.105)), module, BaseOsc::INDEXMODAMT_PARAM));
		addParam(createParamCentered<Rogan1PWhite>(mm2px(Vec(51.913, 81.33)), module, BaseOsc::PULSEWIDTH_PARAM));
		addParam(createParamCentered<Rogan1PWhite>(mm2px(Vec(51.913, 96.407)), module, BaseOsc::INDEX_PARAM));
		addParam(createParamCentered<Rogan1PWhite>(mm2px(Vec(51.913, 111.484)), module, BaseOsc::BITS_PARAM));

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(9.047, 66.718)), module, BaseOsc::VOCT_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(23.336, 66.718)), module, BaseOsc::FM_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(37.624, 66.718)), module, BaseOsc::PWM_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(51.913, 66.718)), module, BaseOsc::INDEXMOD_INPUT));

		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(9.047, 81.33)), module, BaseOsc::TRI_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(23.336, 81.33)), module, BaseOsc::SAW_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(37.624, 81.33)), module, BaseOsc::PULSE_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(9.047, 96.407)), module, BaseOsc::SINE_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(23.336, 96.407)), module, BaseOsc::SUBSQUARE_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(37.624, 96.407)), module, BaseOsc::WAVETABLE_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(9.047, 111.484)), module, BaseOsc::NOISE_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(23.336, 111.484)), module, BaseOsc::PITCHEDNOISE_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(37.624, 111.484)), module, BaseOsc::CLOCKEDNOISE_OUTPUT));

		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(17.441, 14.473)), module, BaseOsc::N5LED_LIGHT));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(20.049, 14.473)), module, BaseOsc::N4LED_LIGHT));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(22.657, 14.473)), module, BaseOsc::N3LED_LIGHT));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(25.265, 14.473)), module, BaseOsc::N2LED_LIGHT));
		addChild(createLightCentered<TinyLight<RedLight>>(mm2px(Vec(27.872, 14.473)), module, BaseOsc::N1LED_LIGHT));
		addChild(createLightCentered<TinyLight<GreenLight>>(mm2px(Vec(30.48, 14.473)), module, BaseOsc::ZEROLED_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(33.088, 14.473)), module, BaseOsc::P1LED_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(35.695, 14.473)), module, BaseOsc::P2LED_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(38.303, 14.473)), module, BaseOsc::P3LED_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(40.911, 14.473)), module, BaseOsc::P4LED_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(43.519, 14.473)), module, BaseOsc::P5LED_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(38.253, 39.442)), module, BaseOsc::QNTLEDCSHARP_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(41.669, 39.442)), module, BaseOsc::QNTLEDDSHARP_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(48.499, 39.442)), module, BaseOsc::QNTLEDFSHARP_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(51.915, 39.442)), module, BaseOsc::QNTLEDGSHARP_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(55.33, 39.442)), module, BaseOsc::QNTLEDASHARP_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(36.546, 43.146)), module, BaseOsc::QNTLEDC_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(39.961, 43.146)), module, BaseOsc::QNTLEDD_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(43.376, 43.146)), module, BaseOsc::QNTLEDE_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(46.792, 43.146)), module, BaseOsc::QNTLEDF_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(50.207, 43.146)), module, BaseOsc::QNTLEDG_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(53.623, 43.146)), module, BaseOsc::QNTLEDA_LIGHT));
		addChild(createLightCentered<TinyLight<RedGreenBlueLight>>(mm2px(Vec(57.038, 43.146)), module, BaseOsc::QNTLEDB_LIGHT));
	}
};


Model* modelBaseOsc = createModel<BaseOsc, BaseOscWidget>("BaseOsc");




