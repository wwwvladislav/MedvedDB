cmake_minimum_required(VERSION 3.2)
project(mdv_platform)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${LIBS_DIR}/binn
    ${LIBS_DIR}/zf_log
    ${LIBS_DIR}/rpmalloc
)

add_library(mdv_platform STATIC ${SOURCES})

add_dependencies(mdv_platform zf_log rpmalloc binn)

target_link_libraries(mdv_platform zf_log rpmalloc binn m)
