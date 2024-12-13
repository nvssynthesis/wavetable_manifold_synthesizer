cmake_minimum_required(VERSION 3.23)

project(wtianns_rtneural VERSION 0.1.0)

juce_add_plugin(${PROJECT_NAME}
#        COPY_PLUGIN_AFTER_BUILD TRUE
        COMPANY_NAME nvssynthesis
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        PLUGIN_MANUFACTURER_CODE NVSS
        PLUGIN_CODE WTNN
        FORMATS VST3 AU Standalone
        PRODUCT_NAME "wtianns"
)

juce_enable_copy_plugin_step(${PROJECT_NAME})

juce_generate_juce_header(${PROJECT_NAME})
add_compile_definitions(EIGEN_STACK_ALLOCATION_LIMIT=1000000)

# Sets the source files of the plugin project.
target_sources(${PROJECT_NAME}
        PRIVATE
        source/PluginEditor.cpp
        source/PluginProcessor.cpp
        source/params.cpp
        source/AttachedSlider.cpp
        source/WMS.cpp
)

# Sets the include directories of the plugin project.
target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LIB_DIR}/RTNeural/RTNeural
        ${LIB_DIR}/chowdsp_fft
        ${LIB_DIR}/fmt/include
)

# Links to all necessary dependencies. The present ones are recommended by JUCE.
# If you use one of the additional modules, like the DSP module, you need to specify it here.
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_basics
        chowdsp::chowdsp_fft_juce
        fmt::fmt
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        RTNeural
)

# These definitions are recommended by JUCE.
target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if (MSVC)
    add_compile_options(/Wall /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()
