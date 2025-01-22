
file(GLOB SRC_LIST "*.cpp")

add_library(${TIR_TARGET} ${SRC_LIST})
# target_link_libraries(${INIT_LIB} PUBLIC )