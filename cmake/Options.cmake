macro(default_option O DEF)
    if (DEFINED ${O})
        if (${${O}})
            set(${O} ON)
        else ()
            set(${O} OFF)
        endif ()
    else ()
        set(${O} ${DEF})
    endif ()
endmacro()
macro(add_sources source_list)
    foreach(source ${source_list})
        add_subdirectory(${source})
    endforeach()
endmacro()

EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)

macro(configure_project)
    set(NAME ${PROJECT_NAME})

    # Default to RelWithDebInfo configuration if no configuration is explicitly specified.
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
                "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    endif ()

    # unit tests
    default_option(TESTS OFF)

    #debug
    default_option(DEBUG OFF)


    # Suffix like "-rc1" e.t.c. to append to versions wherever needed.
    if (NOT DEFINED VERSION_SUFFIX)
        set(VERSION_SUFFIX "")
    endif ()

    ####### options settings ######
    print_config("tiny-dnn-compiler")
endmacro()

macro(print_config NAME)
    message("")
    message("------------------------------------------------------------------------")
    message("-- Configuring ${NAME} ${PROJECT_VERSION}${VERSION_SUFFIX}")
    message("------------------------------------------------------------------------")
    message("-- CMake               CMake version and location   ${CMAKE_VERSION} (${CMAKE_COMMAND})")
    message("-- Compiler            C++ compiler version         ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message("-- CMAKE_BUILD_TYPE    Build type                   ${CMAKE_BUILD_TYPE}")
    message("-- TARGET_PLATFORM     Target platform              ${CMAKE_HOST_SYSTEM_NAME} ${ARCHITECTURE}")
    message("-- TESTS               Build tests                  ${TESTS}")
    message("-- DEBUG                                            ${DEBUG}")
    message("------------------------------------------------------------------------")
    message("")
endmacro()