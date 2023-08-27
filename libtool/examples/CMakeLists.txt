# cmake settings

add_executable(LogDemo LogDemo.cpp)
add_executable(LogDemoLoadCfg LogDemoLoadCfg.cpp)
target_include_directories(LogDemo PRIVATE ..)
target_include_directories(LogDemoLoadCfg PRIVATE ..)

target_link_libraries(LogDemo libtool)
target_link_libraries(LogDemoLoadCfg libtool)