cmake_minimum_required(VERSION 3.2)
project(mdv_api)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${LIBS_DIR}/binn
    ${LIBS_DIR}/nng/include
    ${ROOT_DIR}/mdv_platform
)

add_library(mdv_api STATIC ${SOURCES})

add_dependencies(mdv_api nng mdv_platform)

target_link_libraries(mdv_api nng mdv_platform)
