cmake_minimum_required(VERSION 3.2)
project(ecc)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(ecc STATIC ${SOURCES})

add_library(mdv::ecc ALIAS ecc)

target_include_directories(ecc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
