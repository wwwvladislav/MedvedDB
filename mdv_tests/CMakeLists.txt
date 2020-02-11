cmake_minimum_required(VERSION 3.4)
project(mdv_tests)

add_definitions(-Wall -Wextra)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c) 

include_directories(
    ${ROOT_DIR}/mdv_platform
    ${ROOT_DIR}/mdv_net
    ${ROOT_DIR}/mdv_core
    ${ROOT_DIR}/mdv_types
    ${ROOT_DIR}/mdv_crypto
    ${LIBS_DIR}/zf_log
    ${LIBS_DIR}/binn
)

use_c11()

add_executable(mdv_tests ${SOURCES})

add_dependencies(mdv_tests mdv_platform mdv_core mdv_types mdv_crypto)

target_link_libraries(mdv_tests mdv_platform mdv_core mdv_types mdv_crypto)
