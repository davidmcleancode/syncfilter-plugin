cmake_minimum_required(VERSION 3.22)

project(SyncFilter VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (APPLE)
       set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS" FORCE)
   endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SyncFilter
    COMPANY_NAME "David Test Plugin"
   BUNDLE_ID "com.davidtestplugin.syncfilter"
   PLUGIN_MANUFACTURER_CODE Ycmp
    PLUGIN_CODE Sync
    FORMATS VST3
    PRODUCT_NAME "SyncFilter"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx Filter
)

target_sources(SyncFilter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/CurveEditor.cpp
)

target_compile_definitions(SyncFilter
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SyncFilter
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
