cmake_minimum_required(VERSION 3.2)
project(mdv_types)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${LIBS_DIR}/binn
    ${LIBS_DIR}/zf_log
    ${ROOT_DIR}/mdv_platform
)

add_library(mdv_types STATIC ${SOURCES})

add_dependencies(mdv_types mdv_platform)

target_link_libraries(mdv_types zf_log mdv_platform)
