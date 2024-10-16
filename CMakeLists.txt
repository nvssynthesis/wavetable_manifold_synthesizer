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
#CPMAddPackage("gh:fmtlib/fmt#11.0.2")
#CPMAddPackage("gh:jatinchowdhury18/RTNeural")
#CPMAddPackage(
#        NAME RTNEURAL
#        GITHUB_REPOSITORY jatinchowdhury18/RTNeural
#        SOURCE_DIR ${LIB_DIR}/RTNeural
#)

add_subdirectory(plugin)
add_subdirectory(${LIB_DIR}/RTNeural)

