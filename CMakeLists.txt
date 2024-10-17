cmake_minimum_required(VERSION 3.23)
project(plugin_setup)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for Mac")

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
# Downloads CPM if not already downloaded. CPM is an easy-to-use package manager nicely integrated with CMake.
include(cmake/cpm.cmake)

# This commands downloads AND configures JUCE. It sets up some variables, like JUCE_SOURCE_DIR.
CPMAddPackage(
        NAME JUCE
        GIT_TAG 8.0.2
        VERSION 8.0.2
        GITHUB_REPOSITORY juce-framework/JUCE
        SOURCE_DIR ${LIB_DIR}/juce
)
CPMAddPackage(
        NAME RTNEURAL
        GITHUB_REPOSITORY jatinchowdhury18/RTNeural
        GIT_TAG main
        SOURCE_DIR ${LIB_DIR}/RTNeural
)
CPMAddPackage(
        NAME CHOWDSP_FFT
        GITHUB_REPOSITORY Chowdhury-DSP/chowdsp_fft
        GIT_TAG main
        SOURCE_DIR ${LIB_DIR}/chowdsp_fft
)
CPMAddPackage(
        NAME FMTLIB
        GIT_TAG 11.0.2
        VERSION 11.0.2
        GITHUB_REPOSITORY fmtlib/fmt
        SOURCE_DIR ${LIB_DIR}/fmt
)

add_subdirectory(plugin)

target_compile_options(RTNeural PRIVATE
        -w  # Disable all warnings for GCC/Clang
)

