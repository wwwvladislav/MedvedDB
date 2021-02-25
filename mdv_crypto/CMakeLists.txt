cmake_minimum_required(VERSION 3.2)
project(mdv_crypto)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(mdv_crypto STATIC ${SOURCES})

add_library(mdv::crypto ALIAS mdv_crypto)

target_include_directories(mdv_crypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mdv_crypto PRIVATE mdv::aes mdv::sha3 mdv::ecc)
