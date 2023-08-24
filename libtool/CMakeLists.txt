project(libtool VERSION ${VERSION})

file(GLOB_RECURSE ALL_SRCS libtool/*.cpp)
set(LIB_LIST )

if (NOT ALL_SRCS STREQUAL "")
    add_library(libtool STATIC ${ALL_SRCS})
    target_link_libraries(libtool PUBLIC ${LIB_LIST})
endif()

if(TESTS)
    enable_testing()
    add_subdirectory(uinttest)
endif()