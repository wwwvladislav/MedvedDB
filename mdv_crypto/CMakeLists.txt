cmake_minimum_required(VERSION 3.2)
project(mdv_crypto)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${LIBS_DIR}/aes
    ${LIBS_DIR}/sha3
    ${LIBS_DIR}/ecc
)

add_library(mdv_crypto STATIC ${SOURCES})

target_link_libraries(mdv_crypto aes sha3 ecc)
