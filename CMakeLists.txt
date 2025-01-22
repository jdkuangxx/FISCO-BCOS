cmake_minimum_required(VERSION 3.14)
set(PROJECT_CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE PATH "The cmake path for the library")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_CMAKE_DIR})

# basic settings
include(Options)
configure_project()

include(Version)
project(tiny-dnn-compiler VERSION ${VERSION})

include(CompilerSettings)
include(BuildInfoGenerator)
include(IncludeDirectories)
include(TargetSettings)

add_subdirectory(src/tir)

if (TESTS)
    enable_testing()
    set(CTEST_OUTPUT_ON_FAILURE TRUE)
endif ()