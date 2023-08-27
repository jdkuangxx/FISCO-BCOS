project(libtool VERSION ${VERSION})

file(GLOB_RECURSE ALL_SRCS libtool/*.cpp)
set(LIB_LIST Boost::log)

find_package(Boost REQUIRED log)

add_library(libtool STATIC ${ALL_SRCS})
target_link_libraries(libtool PUBLIC ${LIB_LIST})

if(TESTS)
    enable_testing()
    add_subdirectory(unit-test)
    add_subdirectory(examples)
endif()