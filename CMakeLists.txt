cmake_minimum_required(VERSION 3.14)

if(NOT DEFINED URL_BASE)
    set(URL_BASE "github.com")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(VERSION "0.0.1")
set(VERSION_SUFFIX "")
include(Options)
configure_project()

# vcpkg init
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    find_package(Git REQUIRED)
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- vcpkg WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(DEMO VERSION ${VERSION})

include(CompilerSettings)
include(IncludeDirectories)

add_subdirectory(libtool)
# add_subdirectory(libnetwork)

# 最顶层需要添加enable_testing才可以启用make test
if(TESTS)
    enable_testing()
endif()

print_config("DEMO")