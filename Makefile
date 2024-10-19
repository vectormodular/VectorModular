# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# Define the directory where the plugin should be installed
INSTALL_DIR := ""

# FLAGS will be passed to both the C and C++ compiler
FLAGS += \
	-DTEST \
	-I./eurorack \
	-Wno-unused-local-typedefs

CFLAGS +=
CXXFLAGS += -I./pichenettes-eurorack



# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=





# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)


#add mutable instruments code to the build

SOURCES += pichenettes-eurorack/stmlib/utils/random.cc
SOURCES += pichenettes-eurorack/stmlib/dsp/atan.cc
SOURCES += pichenettes-eurorack/stmlib/dsp/units.cc

SOURCES += pichenettes-eurorack/braids/macro_oscillator.cc
SOURCES += pichenettes-eurorack/braids/analog_oscillator.cc
SOURCES += pichenettes-eurorack/braids/digital_oscillator.cc
SOURCES += pichenettes-eurorack/braids/quantizer.cc
SOURCES += pichenettes-eurorack/braids/resources.cc

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk
