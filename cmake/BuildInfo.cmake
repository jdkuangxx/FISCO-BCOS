function(create_build_info)
    # Set build platform; to be written to BuildInfo.h
    set(TDC_BUILD_OS "${CMAKE_HOST_SYSTEM_NAME}")

    if (CMAKE_COMPILER_IS_MINGW)
        set(TDC_BUILD_COMPILER "mingw")
    elseif (CMAKE_COMPILER_IS_MSYS)
        set(TDC_BUILD_COMPILER "msys")
    elseif (CMAKE_COMPILER_IS_GNUCXX)
        set(TDC_BUILD_COMPILER "g++")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(TDC_BUILD_COMPILER "clang")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        set(TDC_BUILD_COMPILER "appleclang")
    else ()
        set(TDC_BUILD_COMPILER "unknown")
    endif ()

    set(TDC_BUILD_PLATFORM "${TDC_BUILD_OS}/${TDC_BUILD_COMPILER}")


    if (CMAKE_BUILD_TYPE)
        set(_cmake_build_type ${CMAKE_BUILD_TYPE})
    else()
        set(_cmake_build_type "${CMAKE_CFG_INTDIR}")
    endif()
    # Generate header file containing useful build information
    add_custom_target(BuildInfo.h ALL
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -DTDC_SOURCE_DIR="${PROJECT_SOURCE_DIR}/.."
        -DTDC_BUILDINFO_IN="${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/BuildInfo.h.in"
        -DTDC_DST_DIR="${PROJECT_BINARY_DIR}/include"
        -DTDC_CMAKE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/cmake"
        -DTDC_BUILD_TYPE="${_cmake_build_type}"
        -DTDC_BUILD_OS="${TDC_BUILD_OS}"
        -DTDC_BUILD_COMPILER="${TDC_BUILD_COMPILER}"
        -DTDC_BUILD_PLATFORM="${TDC_BUILD_PLATFORM}"
        -DTDC_BUILD_NUMBER="${TDC_BUILD_NUMBER}"
        -DTDC_VERSION_SUFFIX="${VERSION_SUFFIX}"
        -DPROJECT_VERSION="${PROJECT_VERSION}"
        -P "${TDC_SCRIPTS_DIR}/buildinfo.cmake"
        )
    include_directories(BEFORE ${PROJECT_BINARY_DIR})
endfunction()
