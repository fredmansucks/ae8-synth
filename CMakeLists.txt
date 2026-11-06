cmake_minimum_required(VERSION 3.22)
project(AE8 VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  # static runtime so the plugin doesn't need the VC++ redistributable
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# JUCE: pass -DJUCE_DIR=path/to/JUCE, or put a JUCE checkout next to this file
if(NOT JUCE_DIR)
  set(JUCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/JUCE)
endif()
add_subdirectory(${JUCE_DIR} JUCE)

set(AE8_FORMATS "VST3;Standalone" CACHE STRING "Plugin formats to build")

# Brand fonts (Archivo + IBM Plex Mono, SIL Open Font License), compiled into every plugin
juce_add_binary_data(AE8Fonts
  HEADER_NAME AE8Fonts.h
  NAMESPACE AE8Fonts
  SOURCES
    Shared/fonts/Archivo-ExtraBold.ttf
    Shared/fonts/Archivo-SemiBold.ttf
    Shared/fonts/IBMPlexMono-Regular.ttf
    Shared/fonts/IBMPlexMono-SemiBold.ttf)
set_target_properties(AE8Fonts PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(AE8
  COMPANY_NAME "AE-8"
  PRODUCT_NAME "AE-8 POLY"
  PLUGIN_MANUFACTURER_CODE Aeau
  PLUGIN_CODE Ae8s
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Synth
  FORMATS ${AE8_FORMATS})

juce_generate_juce_header(AE8)

target_sources(AE8 PRIVATE
  Source/PluginProcessor.cpp
  Source/PluginEditor.cpp)

target_compile_definitions(AE8 PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AE8
  PRIVATE juce::juce_audio_utils AE8Fonts
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------- AE-8 TUNE
juce_add_plugin(AE8TUNE
  COMPANY_NAME "AE-8"
  PRODUCT_NAME "AE-8 TUNE"
  PLUGIN_MANUFACTURER_CODE Aeau
  PLUGIN_CODE Ae8t
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx "Pitch Shift"
  FORMATS ${AE8_FORMATS})

juce_generate_juce_header(AE8TUNE)

target_sources(AE8TUNE PRIVATE
  Tune/TuneProcessor.cpp
  Tune/TuneEditor.cpp)

target_compile_definitions(AE8TUNE PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AE8TUNE
  PRIVATE juce::juce_audio_utils AE8Fonts
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

juce_add_plugin(AE8TAPE
  COMPANY_NAME "AE-8"
  PRODUCT_NAME "AE-8 TAPE"
  PLUGIN_MANUFACTURER_CODE Aeau
  PLUGIN_CODE Ae8a
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Fx "Distortion"
  FORMATS ${AE8_FORMATS})

juce_generate_juce_header(AE8TAPE)

target_sources(AE8TAPE PRIVATE
  Tape/TapeProcessor.cpp
  Tape/TapeEditor.cpp)

target_compile_definitions(AE8TAPE PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AE8TAPE
  PRIVATE juce::juce_audio_utils AE8Fonts
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

juce_add_plugin(AE8CHOP
  COMPANY_NAME "AE-8"
  PRODUCT_NAME "AE-8 CHOP"
  PLUGIN_MANUFACTURER_CODE Aeau
  PLUGIN_CODE Ae8c
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  VST3_CATEGORIES Instrument Sampler
  FORMATS ${AE8_FORMATS})

juce_generate_juce_header(AE8CHOP)

target_sources(AE8CHOP PRIVATE
  Chop/ChopProcessor.cpp
  Chop/ChopEditor.cpp)

target_compile_definitions(AE8CHOP PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
  JUCE_USE_MP3AUDIOFORMAT=1)

target_link_libraries(AE8CHOP
  PRIVATE juce::juce_audio_utils AE8Fonts
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
