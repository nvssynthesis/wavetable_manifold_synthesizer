cmake_minimum_required(VERSION 3.23)
project(plugin_setup)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for Mac")

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

add_subdirectory(libs/fmt)
add_subdirectory(libs/JUCE)

# Whatever target chowdsp_fft exposes
add_subdirectory(libs/chowdsp_fft)

# RTNeural
add_subdirectory(libs/RTNeural)

add_subdirectory(plugin)

target_compile_options(RTNeural PRIVATE
        -w  # Disable all warnings for GCC/Clang
)

