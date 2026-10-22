cmake_minimum_required(VERSION 3.22)
project(TR909 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.8
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(TR909
  COMPANY_NAME "TonyRando"
  BUNDLE_ID com.tonyrando.tr909
  PLUGIN_MANUFACTURER_CODE Tony
  PLUGIN_CODE T909
  FORMATS VST3 AU
  PRODUCT_NAME "TR-909"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Synth
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

juce_add_binary_data(TR909Data SOURCES
  samples/bd.wav
  samples/sd.wav
  samples/lt.wav
  samples/mt.wav
  samples/ht.wav
  samples/rs.wav
  samples/cp.wav
  samples/ch.wav
  samples/oh.wav
  samples/cr.wav
  samples/rd.wav
)

juce_generate_juce_header(TR909)

target_sources(TR909 PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp
)

target_compile_definitions(TR909 PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(TR909
  PRIVATE
    TR909Data
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
