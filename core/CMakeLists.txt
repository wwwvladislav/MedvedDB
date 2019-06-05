cmake_minimum_required(VERSION 3.2)
project(core)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c) 

use_c11()

include_directories(
    ${LIBS_DIR}/inih
    ${LIBS_DIR}/zf_log
    ${ROOT_DIR}/platform
)

add_library(core STATIC ${SOURCES})
add_dependencies(core inih platform)
target_link_libraries(core inih zf_log platform)
