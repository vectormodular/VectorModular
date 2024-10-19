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
#include "math.hpp"


//declare maxChGainKnobValue
float maxChGainKnobValue = std::sqrt(1.5);

struct SoloMixer : Module {
	enum ParamId {
		SOLORED_PARAM,
		LEVELRED_PARAM,
		SOLOGREEN_PARAM,
		LEVELGREEN_PARAM,
		SOLOBLUE_PARAM,
		LEVELBLUE_PARAM,
		SOLOTOGGLE_PARAM,
		LEVELMIX_PARAM, //bipolar attenuverter
		PARAMS_LEN
	};
	enum InputId {
		RED_INPUT,
		GREEN_INPUT,
		BLUE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		RED_OUTPUT,
		GREEN_OUTPUT,
		BLUE_OUTPUT,
		BMIX_OUTPUT, //attenuverter behavior
		UMIX_OUTPUT, //unipolar, amplitude only behavior, based on the level of the attenuverter output. for example, full negative and full positve = same on this output
		OUTPUTS_LEN
	};
	enum LightIds {

		ENUMS(CHrLED_RGB, 3),
		ENUMS(CHgLED_RGB, 3),
		ENUMS(CHbLED_RGB, 3),
		ENUMS(CHmLED_RGB, 3),
		ToggleFled,
		ToggleTled1,
		ToggleTled2,
		RsoloFled,
		RsoloTled1,
		RsoloTled2,
		GsoloFled,
		GsoloTled1,
		GsoloTled2,
		BsoloFled,
		BsoloTled1,
		BsoloTled2,
		NUM_LIGHTS
		
	};

	SoloMixer() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, NUM_LIGHTS);
		configParam(SOLORED_PARAM, 0.f, 1.f, 0.f, "Solo Red");
		configParam(LEVELRED_PARAM, 0.f, maxChGainKnobValue, 0.f, "Ch. R Level"); // 1 is equal to no change
		configParam(SOLOGREEN_PARAM, 0.f, 1.f, 0.f, "Solo Green");
		configParam(LEVELGREEN_PARAM, 0.f, maxChGainKnobValue, 0.f, "Ch. G Level"); 
		configParam(SOLOBLUE_PARAM, 0.f, 1.f, 0.f, "Solo Blue");
		configParam(LEVELBLUE_PARAM, 0.f, maxChGainKnobValue, 0.f, "Ch. B Level");
		configParam(SOLOTOGGLE_PARAM, 0.f, 1.f, 0.f, "Solo Toggle");
		configParam(LEVELMIX_PARAM, -1.5f, 1.5f, 0.f, "Final Mix Attenuverter"); // linear only
		configInput(RED_INPUT, "Ch. R Input");
		configInput(GREEN_INPUT, "Ch. G");
		configInput(BLUE_INPUT, "Ch. B");
		configOutput(RED_OUTPUT, "Ch. R");
		configOutput(GREEN_OUTPUT, "Ch. G");
		configOutput(BLUE_OUTPUT, "Ch. B");
		configOutput(BMIX_OUTPUT, "Bipolar Final Mix");
		configOutput(UMIX_OUTPUT, "Unipolar Final Mix");
	}

	
	//declare signals
	float redSignal = 0.f;
	float greenSignal = 0.f;
	float blueSignal = 0.f;
	float bMixSignal = 0.f;
	float uMixSignal = 0.f;

	//declare solo statuses
	//solo status when soloToggle state == false
	bool redSoloF = false; 
	bool greenSoloF = false;
	bool blueSoloF = false;
	//solo status when soloToggle state == true
	bool redSoloT = false; 
	bool greenSoloT = false;
	bool blueSoloT = false;
	//toggle between two sets of solo
	bool soloToggle = false;

	//declare solo triggers
	dsp::SchmittTrigger soloRedButton; // solo button
	bool soloRedButtonLastState = false; //last state of this solo button

	dsp::SchmittTrigger soloGreenButton;
	bool soloGreenButtonLastState = false;

	dsp::SchmittTrigger soloBlueButton;
	bool soloBlueButtonLastState = false;

	dsp::SchmittTrigger soloToggleButton;
	bool soloToggleButtonLastState = false;

	//declare channel led brightness

	float chRledBrightness = 0.f;
	float chGledBrightness = 0.f;
	float chBledBrightness = 0.f;

	float chRledMixBrightness = 0.f;
	float chGledMixBrightness = 0.f;
	float chBledMixBrightness = 0.f;

	//declare channel signal to color led brightness function and variables

	float redLEDBuffer = 0.0f;
	float greenLEDBuffer = 0.0f;
	float blueLEDBuffer = 0.0f;

	//declare functions for solo led logic

	void soloTLedStatus(bool soloStatus, int soloLight1, int soloLight2){

			if(soloStatus){
				lights[soloLight1].setBrightness(1.0f);
				lights[soloLight2].setBrightness(1.0f);
			}else{
				lights[soloLight1].setBrightness(0.f);
				lights[soloLight2].setBrightness(0.f);
			}

		}

	void soloFLedStatus(bool soloStatus, int soloLight){

			if(soloStatus){
				lights[soloLight].setBrightness(1.0f);
			}else{
				lights[soloLight].setBrightness(0.f);
			}

		}

	//declare the soft-clipping function. threshold of 5v
	float softClip(float inputSignal, float threshold){
		
		float absV = std::abs(inputSignal); 

		if(absV < threshold){ //within threshold?
			return inputSignal;
		} else{ //apply the soft clipping polynomial
			float clippedSignal = threshold * (0.5f + 0.5f * std::cos(M_PI / 2 * (absV - threshold) / threshold));
			if(inputSignal >= 0){
				return clippedSignal;
			} else{
				return clippedSignal * -1; //retain original polarity
			}
		}
	}
	
	const float rgbThresholdClip = 5.0f;
	const float finalThresholdClip = 10.0f;

	//json input and output.

	json_t* dataToJson() override {
    json_t* rootJ = json_object();
    
    // Save solo statuses
    json_object_set_new(rootJ, "redSoloF", json_boolean(redSoloF));
    json_object_set_new(rootJ, "greenSoloF", json_boolean(greenSoloF));
    json_object_set_new(rootJ, "blueSoloF", json_boolean(blueSoloF));
    json_object_set_new(rootJ, "redSoloT", json_boolean(redSoloT));
    json_object_set_new(rootJ, "greenSoloT", json_boolean(greenSoloT));
    json_object_set_new(rootJ, "blueSoloT", json_boolean(blueSoloT));
    json_object_set_new(rootJ, "soloToggle", json_boolean(soloToggle));

    return rootJ;
}

void dataFromJson(json_t* rootJ) override {
    json_t* redSoloFJ = json_object_get(rootJ, "redSoloF");
    if (redSoloFJ)
        redSoloF = json_is_true(redSoloFJ);
        
    json_t* greenSoloFJ = json_object_get(rootJ, "greenSoloF");
    if (greenSoloFJ)
        greenSoloF = json_is_true(greenSoloFJ);
        
    json_t* blueSoloFJ = json_object_get(rootJ, "blueSoloF");
    if (blueSoloFJ)
        blueSoloF = json_is_true(blueSoloFJ);
        
    json_t* redSoloTJ = json_object_get(rootJ, "redSoloT");
    if (redSoloTJ)
        redSoloT = json_is_true(redSoloTJ);
        
    json_t* greenSoloTJ = json_object_get(rootJ, "greenSoloT");
    if (greenSoloTJ)
        greenSoloT = json_is_true(greenSoloTJ);
        
    json_t* blueSoloTJ = json_object_get(rootJ, "blueSoloT");
    if (blueSoloTJ)
        blueSoloT = json_is_true(blueSoloTJ);
        
    json_t* soloToggleJ = json_object_get(rootJ, "soloToggle");
    if (soloToggleJ)
        soloToggle = json_is_true(soloToggleJ);
    

}
	
	void process(const ProcessArgs& args) override {

		//cascading inputs and also CV offset default.
		redSignal = inputs[RED_INPUT].isConnected() ? inputs[RED_INPUT].getVoltage() : 5.0f;
		greenSignal = inputs[GREEN_INPUT].isConnected() ? inputs[GREEN_INPUT].getVoltage() : redSignal;
		blueSignal = inputs[BLUE_INPUT].isConnected() ? inputs[BLUE_INPUT].getVoltage() : greenSignal;

		// Apply attenuation/gain using std::pow.
    	redSignal *= std::pow(params[LEVELRED_PARAM].getValue(), 2);
		//apply soft clip
		redSignal = softClip(redSignal,rgbThresholdClip);
		//apply clamp 
		redSignal = clamp(redSignal, -10.0f, 10.0f);

    	greenSignal *= std::pow(params[LEVELGREEN_PARAM].getValue(), 2);
		greenSignal = softClip(greenSignal,rgbThresholdClip);
		greenSignal = clamp(greenSignal, -10.0f, 10.0f);

    	blueSignal *= std::pow(params[LEVELBLUE_PARAM].getValue(), 2);
		blueSignal = softClip(blueSignal,rgbThresholdClip);
		blueSignal = clamp(blueSignal, -10.0f, 10.0f); 


		//one day, go back and make this toggle button logic a function to clean all of this redundancy up
		
		// solo button red toggle for each state
		if(soloRedButton.process(params[SOLORED_PARAM].getValue())){ //should be a default value of 1.f to trigger it based on digital.hpp
			if(soloRedButtonLastState){
				return;
			}
			
			if(soloToggle){ //soloToggle state True
					redSoloT = !redSoloT; // shorthand for toggling bool values in c++
			} else {
					redSoloF = !redSoloF;
			}

			soloRedButtonLastState = true; //since the signal was triggered and the toggle was done, this is now true

			} else{ //if the trigger is not above the threshold
				soloRedButtonLastState = false;
			}

		// solo button green toggle for each state	
		if(soloGreenButton.process(params[SOLOGREEN_PARAM].getValue())){
			if(soloGreenButtonLastState){
				return;
			}
			
			if(soloToggle){
					greenSoloT = !greenSoloT;
			} else {
					greenSoloF = !greenSoloF;
			}

			soloGreenButtonLastState = true;

			} else{
				soloGreenButtonLastState = false;
			}

		// solo button blue toggle for each state
		if(soloBlueButton.process(params[SOLOBLUE_PARAM].getValue())){
			if(soloBlueButtonLastState){
				return;
			}
			
			if(soloToggle){
					blueSoloT = !blueSoloT;
			} else {
					blueSoloF = !blueSoloF;
			}

			soloBlueButtonLastState = true;

			} else{
				soloBlueButtonLastState = false;
			}

		//toggle between two solo states
		if(soloToggleButton.process(params[SOLOTOGGLE_PARAM].getValue())){			
			if(soloToggleButtonLastState){
				return;
			} else{
				soloToggle = !soloToggle;
				soloToggleButtonLastState = true; 
			}
		} else{
			soloToggleButtonLastState = false;
		}

	//color output led logic

	// Smooth LED brightness calculations
		auto signalToLEDBrightness = [&](float voltage, float& buffer) {
			float targetBrightness;

			voltage = std::abs(voltage);

			if (voltage >= 5.0f) {
				targetBrightness = 1.0f; // Full brightness for voltages >= 5V
			} else if (voltage > 0.0f && voltage < 5.0f) {
				targetBrightness = voltage / 5.0f; // Interpolate
			} else {
				targetBrightness = 0.0f; // Off for voltages >= 0V
			}

			// Adjusted for more responsive smoothing
			buffer += (targetBrightness - buffer) * 0.001f; // Factor for responsiveness. maye need to play with this if it doesn't look right
			return buffer;
		};


		chRledBrightness = signalToLEDBrightness(redSignal,redLEDBuffer);
		chRledMixBrightness = chRledBrightness;
		
		chGledBrightness = signalToLEDBrightness(greenSignal,greenLEDBuffer);
		chGledMixBrightness = chGledBrightness;

		chBledBrightness = signalToLEDBrightness(blueSignal,blueLEDBuffer);
		chBledMixBrightness = chBledBrightness;		

		lights[CHrLED_RGB + 0].setBrightness(chRledBrightness); // red brightness
		lights[CHrLED_RGB + 1].setBrightness(0.f); // green brightness
		lights[CHrLED_RGB + 2].setBrightness(0.); // blue brightness 

		lights[CHgLED_RGB + 0].setBrightness(0.f); 
		lights[CHgLED_RGB + 1].setBrightness(chGledBrightness); 
		lights[CHgLED_RGB + 2].setBrightness(0.f);

		lights[CHbLED_RGB + 0].setBrightness(0.f); 
		lights[CHbLED_RGB + 1].setBrightness(0.f); 
		lights[CHbLED_RGB + 2].setBrightness(chBledBrightness);
		
		
		//solo LED logic

		if(soloToggle){	
			lights[ToggleTled1].setBrightness(1.0f);
			lights[ToggleTled2].setBrightness(1.0f);
			lights[ToggleFled].setBrightness(0.f);
		}else{
			lights[ToggleTled1].setBrightness(0.f);
			lights[ToggleTled2].setBrightness(0.f);
			lights[ToggleFled].setBrightness(1.0f);
		}

		soloTLedStatus(redSoloT,RsoloTled1,RsoloTled2);
		soloFLedStatus(redSoloF,RsoloFled);

		soloTLedStatus(greenSoloT,GsoloTled1,GsoloTled2);
		soloFLedStatus(greenSoloF,GsoloFled);

		soloTLedStatus(blueSoloT,BsoloTled1,BsoloTled2);
		soloFLedStatus(blueSoloF,BsoloFled);
		
		
		//figure out which channels should be muted, and also the final led output

		int soloCount = 0;

		if(soloToggle){
			soloCount = redSoloT + greenSoloT + blueSoloT; //bool values count as 1
		} else{
			soloCount = redSoloF + greenSoloF + blueSoloF;
		}

		if(soloCount > 0){
			if(soloToggle){
				if(!redSoloT){
					redSignal = 0.f;
					chRledMixBrightness = 0.f;	
				} 
				if(!greenSoloT){
					greenSignal = 0.f;
					chGledMixBrightness = 0.f;	
				}
				if(!blueSoloT){
					blueSignal = 0.f;
					chBledMixBrightness = 0.f;	
				}	
			} else{
				if(!redSoloF){
					redSignal = 0.f;
					chRledMixBrightness = 0.f;	
				} 
				if(!greenSoloF){
					greenSignal = 0.f;
					chGledMixBrightness = 0.f;	
				}
				if(!blueSoloF){
					blueSignal = 0.f;
					chBledMixBrightness = 0.f;	
				}	
			}
		}


		//get final mix signals

		bMixSignal = params[LEVELMIX_PARAM].getValue() * (redSignal + greenSignal + blueSignal); //bipolar mix
		uMixSignal = std::abs(params[LEVELMIX_PARAM].getValue()) * (redSignal + greenSignal + blueSignal); //unipolar mix

		bMixSignal = softClip(bMixSignal,finalThresholdClip);
		bMixSignal = clamp(bMixSignal, -10.0f, 10.0f); 

		uMixSignal = softClip(uMixSignal,finalThresholdClip);
		uMixSignal = clamp(uMixSignal, -10.0f, 10.0f); //i think this being set to zero was messing up the output. maybe relabel + to Uni or something
		
		//mix color based on solos
		
		lights[CHmLED_RGB + 0].setBrightness(chRledMixBrightness); 
		lights[CHmLED_RGB + 1].setBrightness(chGledMixBrightness); 
		lights[CHmLED_RGB + 2].setBrightness(chBledMixBrightness);

		//set outputs
		outputs[RED_OUTPUT].setVoltage(redSignal);
		outputs[GREEN_OUTPUT].setVoltage(greenSignal);
		outputs[BLUE_OUTPUT].setVoltage(blueSignal);
		outputs[BMIX_OUTPUT].setVoltage(bMixSignal);
		outputs[UMIX_OUTPUT].setVoltage(uMixSignal);

	}
};


struct SoloMixerWidget : ModuleWidget {
	SoloMixerWidget(SoloMixer* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/SoloMixer.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<LEDButton>(mm2px(Vec(4.671, 21.148)), module, SoloMixer::SOLORED_PARAM));
		addParam(createParamCentered<Rogan2PSRed>(mm2px(Vec(18.168, 21.148)), module, SoloMixer::LEVELRED_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(4.671, 40.48)), module, SoloMixer::SOLOGREEN_PARAM));
		addParam(createParamCentered<Rogan2PSGreen>(mm2px(Vec(17.934, 40.48)), module, SoloMixer::LEVELGREEN_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(4.671, 59.813)), module, SoloMixer::SOLOBLUE_PARAM));
		addParam(createParamCentered<Rogan2PSBlue>(mm2px(Vec(17.934, 59.813)), module, SoloMixer::LEVELBLUE_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(4.671, 80.643)), module, SoloMixer::SOLOTOGGLE_PARAM));
		addParam(createParamCentered<Rogan3PSWhite>(mm2px(Vec(17.934, 80.643)), module, SoloMixer::LEVELMIX_PARAM));

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(11.645, 100.384)), module, SoloMixer::RED_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(28.996, 100.384)), module, SoloMixer::GREEN_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(11.645, 115.29)), module, SoloMixer::BLUE_INPUT));

		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.445, 21.148)), module, SoloMixer::RED_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.445, 40.48)), module, SoloMixer::GREEN_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.445, 59.813)), module, SoloMixer::BLUE_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(33.445, 80.643)), module, SoloMixer::BMIX_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(28.996, 115.29)), module, SoloMixer::UMIX_OUTPUT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(33.4455, 29.1325)), module, SoloMixer::CHrLED_RGB));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(33.4455, 48.4655)), module, SoloMixer::CHgLED_RGB));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(33.4455, 67.7975)), module, SoloMixer::CHbLED_RGB));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(33.4455, 88.6285)), module, SoloMixer::CHmLED_RGB));

		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(4.671, 74.8225)), module, SoloMixer::ToggleFled));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(3.309,86.4645)), module, SoloMixer::ToggleTled1));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(6.034,86.4645)), module, SoloMixer::ToggleTled2));

		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(4.671,15.3275)), module, SoloMixer::RsoloFled));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(3.309,26.9685)), module, SoloMixer::RsoloTled1));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(6.034,26.9685)), module, SoloMixer::RsoloTled2));

		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(4.671,34.6595)), module, SoloMixer::GsoloFled));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(3.309,46.3015)), module, SoloMixer::GsoloTled1));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(6.034,46.3015)), module, SoloMixer::GsoloTled2));

		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(4.671,53.9925)), module, SoloMixer::BsoloFled));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(3.309,65.6335)), module, SoloMixer::BsoloTled1));
		addChild(createLightCentered<TinyLight<YellowLight>>(mm2px(Vec(6.034,65.6335)), module, SoloMixer::BsoloTled2));
	}
};


Model* modelSoloMixer = createModel<SoloMixer, SoloMixerWidget>("SoloMixer");