cmake_minimum_required(VERSION 3.2)
project(mdv_platform)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${LIBS_DIR}/zf_log
)

add_library(mdv_platform STATIC ${SOURCES})
