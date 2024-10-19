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
#include <cmath> // for logf()

// Utility function to map voltage to a brightness value (0.0 to 1.0 range)
float mapVoltageToBrightness(float voltage, float minVoltage, float maxVoltage) {
    // Normalize voltage to range [0, 1]
    float normalized = (voltage - minVoltage) / (maxVoltage - minVoltage);
    
    // Apply logarithmic scaling
    float brightness = log10(1 + normalized * 9) / log10(10);
    
    return brightness;
}

struct ThreeIx9o : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		INPUT1_INPUT,
		INPUT2_INPUT,
		INPUT3_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT1_OUTPUT,
		OUTPUT2_OUTPUT,
		OUTPUT3_OUTPUT,
		OUTPUT4_OUTPUT,
		OUTPUT5_OUTPUT,
		OUTPUT6_OUTPUT,
		OUTPUT7_OUTPUT,
		OUTPUT8_OUTPUT,
		OUTPUT9_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightIds {
		ENUMS(LED1_RGB, 3),
		ENUMS(LED2_RGB, 3),
		ENUMS(LED3_RGB, 3),
		NUM_LIGHTS
	};

	float led1RedBuffer = 0.0f;
	float led1BlueBuffer = 0.0f;
	float led2RedBuffer = 0.0f;
	float led2BlueBuffer = 0.0f;
	float led3RedBuffer = 0.0f;
	float led3BlueBuffer = 0.0f;

	ThreeIx9o() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, NUM_LIGHTS);
		configInput(INPUT1_INPUT, "");
		configInput(INPUT2_INPUT, "");
		configInput(INPUT3_INPUT, "");
		configOutput(OUTPUT1_OUTPUT, "");
		configOutput(OUTPUT2_OUTPUT, "");
		configOutput(OUTPUT3_OUTPUT, "");
		configOutput(OUTPUT4_OUTPUT, "");
		configOutput(OUTPUT5_OUTPUT, "");
		configOutput(OUTPUT6_OUTPUT, "");
		configOutput(OUTPUT7_OUTPUT, "");
		configOutput(OUTPUT8_OUTPUT, "");
		configOutput(OUTPUT9_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
		// Get input signals
		float input1 = inputs[INPUT1_INPUT].getVoltage();
		float input2 = inputs[INPUT2_INPUT].isConnected() ? inputs[INPUT2_INPUT].getVoltage() : outputs[OUTPUT3_OUTPUT].getVoltage();
		float input3 = inputs[INPUT3_INPUT].isConnected() ? inputs[INPUT3_INPUT].getVoltage() : outputs[OUTPUT6_OUTPUT].getVoltage();

		// Set outputs
		outputs[OUTPUT1_OUTPUT].setVoltage(input1);
		outputs[OUTPUT2_OUTPUT].setVoltage(input1);
		outputs[OUTPUT3_OUTPUT].setVoltage(input1);
		outputs[OUTPUT4_OUTPUT].setVoltage(input2);
		outputs[OUTPUT5_OUTPUT].setVoltage(input2);
		outputs[OUTPUT6_OUTPUT].setVoltage(input2);
		outputs[OUTPUT7_OUTPUT].setVoltage(input3);
		outputs[OUTPUT8_OUTPUT].setVoltage(input3);
		outputs[OUTPUT9_OUTPUT].setVoltage(input3);

		// Smooth LED brightness calculations
		auto mapToRed = [&](float voltage, float& buffer) {
			float targetBrightness;
			if (voltage < -5.0f) {
				targetBrightness = 1.0f; // Full brightness for voltages < -5V
			} else if (voltage >= -5.0f && voltage < 0.0f) {
				targetBrightness = (0.0f - voltage) / 5.0f; // Interpolate
			} else {
				targetBrightness = 0.0f; // Off for voltages >= 0V
			}

			// Adjusted for more responsive smoothing
			buffer += (targetBrightness - buffer) * 0.2f; // Factor for responsiveness
			return buffer;
		};

		auto mapToBlue = [&](float voltage, float& buffer) {
			float targetBrightness;
			if (voltage > 5.0f) {
				targetBrightness = 1.0f; // Full brightness for voltages > 5V
			} else if (voltage <= 5.0f && voltage > 0.0f) {
				targetBrightness = (voltage - 0.0f) / 5.0f; // Interpolate
			} else {
				targetBrightness = 0.0f; // Off for voltages <= 0V
			}

			// Adjusted for more responsive smoothing
			buffer += (targetBrightness - buffer) * 0.2f; // Factor for responsiveness
			return buffer;
		};

		// Compute brightness values for each LED
		float led1Red = mapToRed(input1, led1RedBuffer);
		float led1Blue = mapToBlue(input1, led1BlueBuffer);
		float led2Red = mapToRed(input2, led2RedBuffer);
		float led2Blue = mapToBlue(input2, led2BlueBuffer);
		float led3Red = mapToRed(input3, led3RedBuffer);
		float led3Blue = mapToBlue(input3, led3BlueBuffer);

		// Set LED colors (Red, Green, Blue)
		lights[LED1_RGB + 0].setBrightness(led1Red); // red brightness
		lights[LED1_RGB + 1].setBrightness(0.0f); // green brightness
		lights[LED1_RGB + 2].setBrightness(led1Blue); // blue brightness 

		lights[LED2_RGB + 0].setBrightness(led2Red);
		lights[LED2_RGB + 1].setBrightness(0.0f);
		lights[LED2_RGB + 2].setBrightness(led2Blue);

		lights[LED3_RGB + 0].setBrightness(led3Red);
		lights[LED3_RGB + 1].setBrightness(0.0f);
		lights[LED3_RGB + 2].setBrightness(led3Blue);
	}
};

struct ThreeIx9oWidget : ModuleWidget {
	ThreeIx9oWidget(ThreeIx9o* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ThreeIx9o.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<CL1362Port>(mm2px(Vec(5.663, 16.907)), module, ThreeIx9o::INPUT1_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(5.663, 54.114)), module, ThreeIx9o::INPUT2_INPUT));
		addInput(createInputCentered<CL1362Port>(mm2px(Vec(5.663, 91.322)), module, ThreeIx9o::INPUT3_INPUT));

		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(14.687, 26.707)), module, ThreeIx9o::OUTPUT1_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(5.796, 32.014)), module, ThreeIx9o::OUTPUT2_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(14.708, 37.231)), module, ThreeIx9o::OUTPUT3_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(14.687, 63.915)), module, ThreeIx9o::OUTPUT4_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(5.796, 69.222)), module, ThreeIx9o::OUTPUT5_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(14.708, 74.439)), module, ThreeIx9o::OUTPUT6_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(14.687, 101.123)), module, ThreeIx9o::OUTPUT7_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(5.796, 106.429)), module, ThreeIx9o::OUTPUT8_OUTPUT));
		addOutput(createOutputCentered<CL1362Port>(mm2px(Vec(14.708, 111.647)), module, ThreeIx9o::OUTPUT9_OUTPUT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(14.687, 16.907)), module, ThreeIx9o::LED1_RGB));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(14.687, 54.114)), module, ThreeIx9o::LED2_RGB));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(14.687, 91.322)), module, ThreeIx9o::LED3_RGB));
	}
};

Model* modelThreeIx9o = createModel<ThreeIx9o, ThreeIx9oWidget>("ThreeIx9o");
