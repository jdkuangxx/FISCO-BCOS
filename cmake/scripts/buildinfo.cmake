macro(replace_if_different SOURCE DST)
    set(extra_macro_args ${ARGN})
    set(options CREATE)
    set(one_value_args)
    set(multi_value_args)
    cmake_parse_arguments(REPLACE_IF_DIFFERENT "${options}" "${one_value_args}" "${multi_value_args}" "${extra_macro_args}")

    if (REPLACE_IF_DIFFERENT_CREATE AND (NOT (EXISTS "${DST}")))
        file(WRITE "${DST}" "")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files "${SOURCE}" "${DST}" RESULT_VARIABLE DIFFERENT OUTPUT_QUIET ERROR_QUIET)

    if (DIFFERENT)
        execute_process(COMMAND ${CMAKE_COMMAND} -E rename "${SOURCE}" "${DST}")
    else()
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove "${SOURCE}")
    endif()
endmacro()

if (NOT TDC_BUILD_TYPE)
    set(TDC_BUILD_TYPE "unknown")
endif()

if (NOT TDC_BUILD_PLATFORM)
    set(TDC_BUILD_PLATFORM "unknown")
endif()

execute_process(
    COMMAND git --git-dir=${TDC_SOURCE_DIR}/.git --work-tree=${TDC_SOURCE_DIR} rev-parse HEAD
    OUTPUT_VARIABLE TDC_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
) 

if (NOT TDC_COMMIT_HASH)
    set(TDC_COMMIT_HASH 0)
endif()

execute_process(
    COMMAND git --git-dir=${TDC_SOURCE_DIR}/.git --work-tree=${TDC_SOURCE_DIR} diff HEAD --shortstat
    OUTPUT_VARIABLE TDC_LOCAL_CHANGES OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
)

execute_process(
    COMMAND date "+%Y%m%d %H:%M:%S" 
    OUTPUT_VARIABLE TDC_BUILD_TIME OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )

execute_process(
    COMMAND date "+%Y%m%d" 
    OUTPUT_VARIABLE TDC_BUILD_NUMBER OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )

execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD 
    OUTPUT_VARIABLE TDC_BUILD_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
    )

if (TDC_LOCAL_CHANGES)
    set(TDC_CLEAN_REPO 0)
else()
    set(TDC_CLEAN_REPO 1)
endif()

set(TMPFILE "${TDC_DST_DIR}/BuildInfo.h.tmp")
set(OUTFILE "${TDC_DST_DIR}/BuildInfo.h")

configure_file("${TDC_BUILDINFO_IN}" "${TMPFILE}")

replace_if_different("${TMPFILE}" "${OUTFILE}" CREATE)

