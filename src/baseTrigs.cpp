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
#include <rack.hpp>

// this version tracks 1/16th steps as the basis for the rest of the outputs. trying to minimize drift.
// triplets will just use the interval calculations
// use PulseGenerator to manage bool state of pulse duration, per output, to have a 5ms output for each trigger

struct BaseTrigs : Module {
	enum ParamId {
		TEMPO_MOD_ATTEN_PARAM, 
		TEMPO_KNOB_PARAM, 
		RESET_BUTTON_PARAM,
		TAP_TEMPO_BUTTON_PARAM, 
		PARAMS_LEN
	};
	enum InputId {
		TEMP_MOD_IN_INPUT, 
		RESET_TRIG_IN_INPUT, 
		CLOCK_TRIG_IN_INPUT, 
		INPUTS_LEN
	};
	enum OutputId {
		_1_16_OUT_OUTPUT, 
		_1_8T_OUT_OUTPUT,
		_1_8_OUT_OUTPUT,
		_1_8_OFFBEAT_OUT_OUTPUT,
		_1_4T_OUT_OUTPUT,
		_1_4_OUT_OUTPUT,
		_1_4_OFFBEAT_OUT_OUTPUT,
		_1_2_OUT_OUTPUT,
		_1_2_OFFBEAT_OUT_OUTPUT,
		_3_4_OUT_OUTPUT,
		_1_1_OUT_OUTPUT,
		_5_4_OUT_OUTPUT, 
		_6_4_OUT_OUTPUT, 
		_7_4_OUT_OUTPUT, 
		OUTPUTS_LEN
	};
	enum LightId {
		_1_4_CLOCK_LED_LIGHT,
		LIGHTS_LEN
	};

	// these PulseGenerator objects are used to manage the timing of the trig ouput, per output
	dsp::PulseGenerator pG_1_16; 
	dsp::PulseGenerator pG_1_8T;
	dsp::PulseGenerator pG_1_8;
	dsp::PulseGenerator pG_1_8OB; // OB == offbeat
	dsp::PulseGenerator pG_1_4T;
	dsp::PulseGenerator pG_1_4;
	dsp::PulseGenerator pG_1_4OB;
	dsp::PulseGenerator pG_1_2;
	dsp::PulseGenerator pG_1_2OB;
	dsp::PulseGenerator pG_3_4;
	dsp::PulseGenerator pG_1_1;
	dsp::PulseGenerator pG_5_4;
	dsp::PulseGenerator pG_6_4;
	dsp::PulseGenerator pG_7_4;
	
	//define SchmittTriggers for reset and clock input

	dsp::SchmittTrigger resetTrigger; // reset input and reset button
	float resetVoltage = 0; // variable to hold reset voltage trigger, so that both reset inputs and reset buttons work the same
	
	dsp::SchmittTrigger clockInput; // clock input
	float currentClockTime = 0.f; 
	bool firstClock = true;

	dsp::SchmittTrigger tapTempoInput; // tap tempo input
	float currentTapTime = 0.f; 
	bool firstTap = true;

	/*
	
	some note I found about how to generate and handle trigs:
	
	Therefore, trigger inputs in Rack should be triggered by a Schmitt trigger with a low threshold of about 0.1 V and a high threshold of around 1 to 2 V.
	Rack plugins can implement this using dsp::SchmittTrigger with schmittTrigger.process(x, 0.1f, 1.f)*/

	//need timers for the trigger inputs as well? I don't think so, because it is not a sequencer being moved by clock, only the bpm is being adjusted by it. keep in for reference just in case.

	// dsp::Timer resetTrigTimer;
	// dsp::Timer clockInputTrigTimer;

	/* Timing: Each cable in Rack induces a 1-sample delay of its carried signal from the output port to the input port.
	This means that it is not guaranteed that two signals generated simultaneously will arrive at their destinations
	at the same time if the number of cables in each signal’s chain is different.
	For example, a pulse sent through a utility module and then to a sequencer’s CLOCK input will arrive one sample
	later than the same pulse sent directly to the sequencer’s RESET input.
	This will cause the sequencer to reset to step 1, and one sample later, advance to step 2, which is undesired behavior.
	Therefore, modules with a CLOCK and RESET input, or similar variants, should ignore CLOCK triggers up to 1 ms after receiving a RESET trigger.
	You can use dsp::Timer for keeping track of time. */


	BaseTrigs() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(TEMPO_MOD_ATTEN_PARAM, 0.f, 1.f, 0.f, "Tempo Mod attenuator");
		configParam(TEMPO_KNOB_PARAM, 0.f, 300.f, 115.f, "knobBPM"); //115 BPM as default purely to line up knob with panel graphic
		configParam(RESET_BUTTON_PARAM, 0.f, 2.f, 0.f, "Reset outputs button");
		configParam(TAP_TEMPO_BUTTON_PARAM, 0.f, 2.f, 0.f, "Tap Tempo button");
		configInput(TEMP_MOD_IN_INPUT, "Tempo Mod input");
		configInput(RESET_TRIG_IN_INPUT, "Reset outputs trig input");
		configInput(CLOCK_TRIG_IN_INPUT, "Clock Trig input");
		configOutput(_1_16_OUT_OUTPUT, "1/16th note output");
		configOutput(_1_8T_OUT_OUTPUT, "1/8th triplet note output");
		configOutput(_1_8_OUT_OUTPUT, "1/8th note output");
		configOutput(_1_8_OFFBEAT_OUT_OUTPUT, "1/8th offbeat note output"); 
		configOutput(_1_4T_OUT_OUTPUT, "1/4th triplet note output");
		configOutput(_1_4_OUT_OUTPUT, "1/4t note nutput / Clock output");
		configOutput(_1_4_OFFBEAT_OUT_OUTPUT, "1/4th offbeat note output");
		configOutput(_1_2_OUT_OUTPUT, "1/2 note output");
		configOutput(_1_2_OFFBEAT_OUT_OUTPUT, "1/2 offbeat note output");
		configOutput(_3_4_OUT_OUTPUT, "3/4th note output");
		configOutput(_1_1_OUT_OUTPUT, "Whole note output");
		configOutput(_5_4_OUT_OUTPUT, "5/4 note output");
		configOutput(_6_4_OUT_OUTPUT, "6/4 note output");
		configOutput(_7_4_OUT_OUTPUT, "7/4 note output");
	}

// Variables to track the clock LED state and timing
bool ledOn = false;
float ledTimer = 0.0f;
const float LED_ON_DURATION = 0.050f; // 50 ms in seconds

// Time tracking for the clocks, where 1/16th is the basis of all non-triplet outputs
float _1_16_clockTime = 0.f;
float _1_8T_clockTime = 0.f;
float _1_4T_clockTime = 0.f;

//variables for bpm, starting at 1 to prevent divide by zero errors
float knobBPM = 115.f; // bpm set by the knob
float lastKnobBPM = 115.f; // track bpm from last process. if different than previous process, the knob moved 	
float bpm = 115.f; // active bpm that is used for everything
float lastGoodBPM = 115.f; // for tracking last known good BPM
float clockOutFallbackBPM = 115.f; // if clock input is unplugged, this bpm takes over

int sixteenthStep = 0;
int _3_4_BeatTrack = 0; //used for the 3/4 reset
int _5_4_BeatTrack = 0; //used for the 5/4 reset
int _6_4_BeatTrack = 0; //used for the 6/4 reset
int _7_4_BeatTrack = 0; //used for the 7/4 reset

//define reset function
void resetOutputs(){
	
	//reset LED
	ledOn = false;
	ledTimer = 0.0f;
	lights[_1_4_CLOCK_LED_LIGHT].setBrightness(0.f);
	
	//reset variables that take place in first measure
	_1_16_clockTime = 0.f;
	_1_8T_clockTime = 0.f;
	_1_4T_clockTime = 0.f;

	//reset sixteenthStep
	sixteenthStep = 0;

	//reset polyrhythmic outputs
	_3_4_BeatTrack = 0; //used for the 3/4 reset
	_5_4_BeatTrack = 0; //used for the 5/4 reset
	_6_4_BeatTrack = 0; //used for the 6/4 reset
	_7_4_BeatTrack = 0; //used for the 7/4 reset

	//reset all pulses

	pG_1_16.reset(); 
	pG_1_8T.reset();
	pG_1_8.reset();
	pG_1_8OB.reset(); //OB == offbeat
	pG_1_4T.reset();
	pG_1_4.reset();
	pG_1_4OB.reset();
	pG_1_2.reset();
	pG_1_2OB.reset();
	pG_3_4.reset();
	pG_1_1.reset();
	pG_5_4.reset();
	pG_6_4.reset();
	pG_7_4.reset();

	// turn off all outputs
    outputs[_1_16_OUT_OUTPUT].setVoltage(0.f);
    outputs[_1_8T_OUT_OUTPUT].setVoltage(0.f);
    outputs[_1_8_OUT_OUTPUT].setVoltage(0.f);
	outputs[_1_8_OFFBEAT_OUT_OUTPUT].setVoltage(0.f);
    outputs[_1_4T_OUT_OUTPUT].setVoltage(0.f);
    outputs[_1_4_OUT_OUTPUT].setVoltage(0.f);
	outputs[_1_4_OFFBEAT_OUT_OUTPUT].setVoltage(0.f);
    outputs[_1_2_OUT_OUTPUT].setVoltage(0.f);
	outputs[_1_2_OFFBEAT_OUT_OUTPUT].setVoltage(0.f);
    outputs[_3_4_OUT_OUTPUT].setVoltage(0.f);
    outputs[_1_1_OUT_OUTPUT].setVoltage(0.f);
	outputs[_5_4_OUT_OUTPUT].setVoltage(0.f);
    outputs[_6_4_OUT_OUTPUT].setVoltage(0.f);
    outputs[_7_4_OUT_OUTPUT].setVoltage(0.f);

}

//make sure tap tempo persists across sessions
json_t* dataToJson() override {
    json_t* rootJ = json_object();

    // Save BPM variables
    json_object_set_new(rootJ, "lastKnobBPM", json_real(lastKnobBPM));
    json_object_set_new(rootJ, "bpm", json_real(bpm));
    json_object_set_new(rootJ, "lastGoodBPM", json_real(lastGoodBPM));
    json_object_set_new(rootJ, "clockOutFallbackBPM", json_real(clockOutFallbackBPM));

    return rootJ;
}

void dataFromJson(json_t* rootJ) override {
    // Load BPM variables
    json_t* lastKnobBPMJ = json_object_get(rootJ, "lastKnobBPM");
    if (lastKnobBPMJ)
        lastKnobBPM = json_real_value(lastKnobBPMJ);

    json_t* bpmJ = json_object_get(rootJ, "bpm");
    if (bpmJ)
        bpm = json_real_value(bpmJ);

    json_t* lastGoodBPMJ = json_object_get(rootJ, "lastGoodBPM");
    if (lastGoodBPMJ)
        lastGoodBPM = json_real_value(lastGoodBPMJ);

    json_t* clockOutFallbackBPMJ = json_object_get(rootJ, "clockOutFallbackBPM");
    if (clockOutFallbackBPMJ)
        clockOutFallbackBPM = json_real_value(clockOutFallbackBPMJ);
}




	void process(const ProcessArgs& args) override {
  	

	// clock input tempo bpm

	if(clockInput.process(inputs[CLOCK_TRIG_IN_INPUT].getVoltage(),0.1f,1.5f)){
			if(firstClock){
				firstClock = false;
				currentClockTime = 0;
			} else if (currentClockTime > 60.f){
				firstClock = true;
				currentClockTime = 0.f;
			} else if(currentClockTime >= 0.001f){ // a little crazy but trying to let users push it
				bpm = 60.f / currentClockTime;
				lastGoodBPM = bpm;
				firstClock = true;
			} else{
				firstClock = true;
				currentClockTime = 0.f;
			}
		}

		if(currentClockTime > 70.f){
			currentClockTime = 0.f;
		}
		currentClockTime += args.sampleTime;	

		
	// tap tempo input

		if(tapTempoInput.process(params[TAP_TEMPO_BUTTON_PARAM].getValue(),0.1f,1.5f)){
			if(firstTap){
				firstTap = false;
				currentTapTime = 0;
			} else if (currentTapTime > 60.f){
				firstTap = true;
				currentTapTime = 0.f;
			} else if(currentTapTime >= 0.01f){
				bpm = 60.f / currentTapTime;
				lastGoodBPM = bpm;
				clockOutFallbackBPM = bpm;
				firstTap = true;
			} else{
				firstTap = true;
				currentTapTime = 0.f;
			}
		}

		if(currentTapTime > 70.f){
			currentTapTime = 0.f;
		}
		currentTapTime += args.sampleTime;	
	
	
	
	// Get the tempo from the knob
    knobBPM = params[TEMPO_KNOB_PARAM].getValue();
	if(knobBPM != lastKnobBPM){
		bpm = knobBPM; 
		lastGoodBPM = bpm;
		lastKnobBPM = knobBPM;
		clockOutFallbackBPM = knobBPM;
	}

	if(!inputs[CLOCK_TRIG_IN_INPUT].isConnected()){
		bpm = clockOutFallbackBPM;
	}

  // Bypass clock generation if bpm is zero. Stop all outputs and lights.
    if (bpm <= 0.f) {
        // Set all outputs to 0V
        for (int i = 0; i < OUTPUTS_LEN; i++) {
            outputs[i].setVoltage(0.f);
        }
        // Turn off all lights
        for (int i = 0; i < LIGHTS_LEN; i++) {
            lights[i].setBrightness(0.f);
        }
        return;  // Skip the rest of the process function
    }
		
		//reset outputs logic

		if(params[RESET_BUTTON_PARAM].getValue() > 1.5f || inputs[RESET_TRIG_IN_INPUT].getVoltage() > 1.5f){
			resetVoltage = 1.6f;
		} else {
			resetVoltage = 0.0f;
		}	

		if(resetTrigger.process(resetVoltage,0.1f, 1.5f)){
			 resetOutputs();
		}

		//bpm mod via tempo mod input
		if(inputs[TEMP_MOD_IN_INPUT].isConnected()){
			float tempoModInput = inputs[TEMP_MOD_IN_INPUT].getVoltage();
			float attenuatedMod = tempoModInput * params[TEMPO_MOD_ATTEN_PARAM].getValue(); // knob goes from 0 to 1

			if(inputs[CLOCK_TRIG_IN_INPUT].isConnected()){

				if(attenuatedMod > 0){
				bpm = lastGoodBPM*(1+attenuatedMod);
 				} else if (attenuatedMod < 0){
				bpm = lastGoodBPM/(-1*(attenuatedMod-1));
				} else{
				bpm = lastGoodBPM;
				}
			} else if(!inputs[CLOCK_TRIG_IN_INPUT].isConnected()){
				if(attenuatedMod > 0){
				bpm = clockOutFallbackBPM*(1+attenuatedMod);
 				} else if (attenuatedMod < 0){
				bpm = clockOutFallbackBPM/(-1*(attenuatedMod-1));
				} else{
				bpm = clockOutFallbackBPM;
				}
			}
			
			

			bpm = clamp(bpm, 0.f, 1000000.f);
		}
		
		//declare intervals

		float _1_16_interval = 15.f / bpm;
		float _1_8T_interval = 20.f / bpm;
		float _1_4T_interval = 40.f / bpm;

		//declare pulse duration
		float pulseDuration = 0.001;  // Made this 1ms per VCV's voltage standards. This variable could be used for gate length in the future.

 		// Store sample time
		float sampleTimeToAdd = args.sampleTime;
		
		//sample time to deltaTime for the pulse check process
		float pulseDeltaTime = sampleTimeToAdd; 

		// Increment clock times
		_1_16_clockTime += sampleTimeToAdd;
		_1_8T_clockTime += sampleTimeToAdd;
		_1_4T_clockTime += sampleTimeToAdd;

	// Evaluate each output

	// Check if the current clock time has passed the interval for the 1/16 note

	if(_1_16_clockTime >= _1_16_interval) {
    	_1_16_clockTime = fmod(_1_16_clockTime, _1_16_interval);

		pG_1_16.trigger(pulseDuration); //trigger pulse
		sixteenthStep += 1;

		if(sixteenthStep > 16){ //wraparound
			sixteenthStep = 1;
		}

		//try doing these in terms of 16th steps, and updating the trigger / reset in those terms 
		_3_4_BeatTrack += 1; //used for the 3/4 reset. 1 = 1/16, since it is float
		_5_4_BeatTrack += 1; //used for the 5/4 reset. 1 = 1/16, since it is float
		_6_4_BeatTrack += 1; //used for the 6/4 reset. 1 = 1/16, since it is float
		_7_4_BeatTrack += 1; //used for the 7/4 reset. 1 = 1/16, since it is float

		
		//wraparound
		if(_3_4_BeatTrack > 12){ //12 = 3 beats
			_3_4_BeatTrack = 1;
		}
		if(_5_4_BeatTrack > 20){ 
			_5_4_BeatTrack = 1;
		}
		if(_6_4_BeatTrack > 24){
			_6_4_BeatTrack = 1;
		}
		if(_7_4_BeatTrack > 28){
			_7_4_BeatTrack = 1;
		}


	} 

	//check if 1/16 pulse is active. if so, set output high. if not, set output low.

	if(pG_1_16.process(pulseDeltaTime)){
		outputs[_1_16_OUT_OUTPUT].setVoltage(10.f);
	} else {
		outputs[_1_16_OUT_OUTPUT].setVoltage(0.f);
	}

	//same idea as above, but for 1/8th triplets, which will also drive 1/4 triplets
	
	if (_1_8T_clockTime >= _1_8T_interval) {
    	_1_8T_clockTime = fmod(_1_8T_clockTime, _1_8T_interval);

		pG_1_8T.trigger(pulseDuration); //trigger pulse
	} 

	//check if 1/8T pulse is active. if so, set output high. if not, set output low.

	if(pG_1_8T.process(pulseDeltaTime)){
		outputs[_1_8T_OUT_OUTPUT].setVoltage(10.f);
	} else {
		outputs[_1_8T_OUT_OUTPUT].setVoltage(0.f);
	}


	//using 16th step counter, trigger other outputs

	if(sixteenthStep % 2 == 1){ // 1/8th note
		pG_1_8.trigger(pulseDuration);
	}
	if(pG_1_8.process(pulseDeltaTime)){
		outputs[_1_8_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_8_OUT_OUTPUT].setVoltage(0.f);
	}

	if(sixteenthStep % 2 == 0){ // 1/8th note, offbeat
		pG_1_8OB.trigger(pulseDuration);
	}
	if(pG_1_8OB.process(pulseDeltaTime)){
		outputs[_1_8_OFFBEAT_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_8_OFFBEAT_OUT_OUTPUT].setVoltage(0.f);
	}

	
	//need to use interval method for 1/4T
	if(_1_4T_clockTime >= _1_4T_interval) {
    	_1_4T_clockTime = fmod(_1_4T_clockTime,_1_4T_interval);
		pG_1_4T.trigger(pulseDuration);
	}
	if(pG_1_4T.process(pulseDeltaTime)){
		outputs[_1_4T_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_4T_OUT_OUTPUT].setVoltage(0.f);
	}

	
	if(sixteenthStep == 1 || sixteenthStep == 5 || sixteenthStep == 9 || sixteenthStep == 13) {
    	pG_1_4.trigger(pulseDuration);
		 // Start the LED timer
  		ledOn = true;
        ledTimer = LED_ON_DURATION;
	}
	if(pG_1_4.process(pulseDeltaTime)){
		outputs[_1_4_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_4_OUT_OUTPUT].setVoltage(0.f);
	}


    // Update the LED state based on the timer
    if (ledOn) {
        ledTimer -= sampleTimeToAdd;
		
        if (ledTimer <= 0.0f) {
            ledOn = false;  // Turn off LED after 50 ms
        }
    }

	lights[_1_4_CLOCK_LED_LIGHT].setBrightness(ledOn ? 1.f : 0.f);

    //continue with outputs

	//1/4 offbeat
	if(sixteenthStep == 3 || sixteenthStep == 7 || sixteenthStep == 11 || sixteenthStep == 15) {
    	pG_1_4OB.trigger(pulseDuration);
	}	
	if(pG_1_4OB.process(pulseDeltaTime)){
		outputs[_1_4_OFFBEAT_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_4_OFFBEAT_OUT_OUTPUT].setVoltage(0.f);
	}	
	
	// 1/2 output
	if(sixteenthStep == 1 || sixteenthStep == 9) {
    	pG_1_2.trigger(pulseDuration);
	}	
	if(pG_1_2.process(pulseDeltaTime)){
		outputs[_1_2_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_2_OUT_OUTPUT].setVoltage(0.f);
	}	

	// 1/2 offbeat output

	if(sixteenthStep == 5 || sixteenthStep == 13) {
    	pG_1_2OB.trigger(pulseDuration);
	}	
	if(pG_1_2OB.process(pulseDeltaTime)){
		outputs[_1_2_OFFBEAT_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_2_OFFBEAT_OUT_OUTPUT].setVoltage(0.f);
	}	

	// 3/4 output
	if(_3_4_BeatTrack == 1){ 
		pG_3_4.trigger(pulseDuration);
	}
	if(pG_3_4.process(pulseDeltaTime)){
		outputs[_3_4_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_3_4_OUT_OUTPUT].setVoltage(0.f);
	}

	// 1/1 output
	if(sixteenthStep == 1) {
    	pG_1_1.trigger(pulseDuration);
	}	
	if(pG_1_1.process(pulseDeltaTime)){
		outputs[_1_1_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_1_1_OUT_OUTPUT].setVoltage(0.f);
	}

	// 5/4 output
	if(_5_4_BeatTrack == 1){ 
		pG_5_4.trigger(pulseDuration);
	}
	if(pG_5_4.process(pulseDeltaTime)){
		outputs[_5_4_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_5_4_OUT_OUTPUT].setVoltage(0.f);
	}

	// 6/4 output
	if(_6_4_BeatTrack == 1){
		pG_6_4.trigger(pulseDuration);
	}
	if(pG_6_4.process(pulseDeltaTime)){
		outputs[_6_4_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_6_4_OUT_OUTPUT].setVoltage(0.f);
	}

	// 7/4 output
	if(_7_4_BeatTrack == 1){ 
		pG_7_4.trigger(pulseDuration);
	}
	if(pG_7_4.process(pulseDeltaTime)){
		outputs[_7_4_OUT_OUTPUT].setVoltage(10.f);
	} else{
		outputs[_7_4_OUT_OUTPUT].setVoltage(0.f);
	}

}
};


struct BaseTrigsWidget : ModuleWidget {
	BaseTrigsWidget(BaseTrigs* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/baseTrigs.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(33.287, 81.48)), module, BaseTrigs::TEMPO_MOD_ATTEN_PARAM));
		addParam(createParamCentered<Rogan1PSWhite>(mm2px(Vec(33.287, 106.571)), module, BaseTrigs::TEMPO_KNOB_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(7.353, 117.797)), module, BaseTrigs::RESET_BUTTON_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(20.32, 117.797)), module, BaseTrigs::TAP_TEMPO_BUTTON_PARAM));
		

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(33.287, 89.75)), module, BaseTrigs::TEMP_MOD_IN_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(7.353, 106.75)), module, BaseTrigs::RESET_TRIG_IN_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(20.32, 106.75)), module, BaseTrigs::CLOCK_TRIG_IN_INPUT));

		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(7.353, 21.75)), module, BaseTrigs::_1_16_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(20.32, 21.75)), module, BaseTrigs::_1_8T_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.287, 21.75)), module, BaseTrigs::_1_8_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(7.353, 38.75)), module, BaseTrigs::_1_8_OFFBEAT_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(20.32, 38.75)), module, BaseTrigs::_1_4T_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.287, 38.75)), module, BaseTrigs::_1_4_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(7.353, 55.75)), module, BaseTrigs::_1_4_OFFBEAT_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(20.32, 55.75)), module, BaseTrigs::_1_2_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.287, 55.75)), module, BaseTrigs::_1_2_OFFBEAT_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(7.353, 72.75)), module, BaseTrigs::_3_4_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(20.32, 72.75)), module, BaseTrigs::_1_1_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.287, 72.75)), module, BaseTrigs::_5_4_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(7.353, 89.75)), module, BaseTrigs::_6_4_OUT_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(20.32, 89.75)), module, BaseTrigs::_7_4_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(26.804, 30.25)), module, BaseTrigs::_1_4_CLOCK_LED_LIGHT));
	}
};


Model* modelBaseTrigs = createModel<BaseTrigs, BaseTrigsWidget>("baseTrigs");

