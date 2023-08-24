project(libnetwork VERSION ${VERSION})

file(GLOB_RECURSE ALL_SRCS libnetwork/*.cpp)
set(LIB_LIST )

if (NOT ALL_SRCS STREQUAL "")
    add_library(libnetwork STATIC ${ALL_SRCS})
    target_link_libraries(libnetwork PUBLIC ${LIB_LIST})
endif()


if(TESTS)
    enable_testing()
    add_subdirectory(uinttest)
endif()