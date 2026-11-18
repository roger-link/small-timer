cmake_minimum_required(VERSION 3.22)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS" FORCE)
endif()
project(SmallTimer VERSION 0.2.2 LANGUAGES C CXX OBJCXX)
set(CMAKE_CXX_STANDARD 17)
set(JUCE_PATH "" CACHE PATH "Path to a JUCE 8 checkout")
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Configure with -DJUCE_PATH=/path/to/JUCE")
endif()
add_subdirectory("${JUCE_PATH}" "${CMAKE_BINARY_DIR}/JUCE")
juce_add_plugin(SmallTimer
    COMPANY_NAME "Small Instruments"
    BUNDLE_ID "audio.smallinstruments.smalltimer"
    PLUGIN_MANUFACTURER_CODE Smin
    PLUGIN_CODE Smtr
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Small Timer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Small Timer uses audio input for monitoring and temporary recording.")
target_sources(SmallTimer PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(SmallTimer PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SmallTimer PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
set_target_properties(SmallTimer PROPERTIES C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden OBJCXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
foreach(format AU VST3 Standalone)
    set_target_properties(SmallTimer_${format} PROPERTIES C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden OBJCXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
    add_custom_command(TARGET SmallTimer_${format} POST_BUILD
        COMMAND /usr/bin/codesign --force --sign - --timestamp=none "$<TARGET_BUNDLE_DIR:SmallTimer_${format}>"
        VERBATIM)
endforeach()
enable_testing()
juce_add_console_app(SmallTimerTests PRODUCT_NAME "Small Timer Tests")
target_sources(SmallTimerTests PRIVATE Tests/Tests.cpp Tests/RegisterAU.mm Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(SmallTimerTests PRIVATE Source)
target_compile_definitions(SmallTimerTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=1)
target_link_libraries(SmallTimerTests PRIVATE juce::juce_audio_utils "-framework AudioToolbox")
add_test(NAME SmallTimerTests COMMAND SmallTimerTests)
