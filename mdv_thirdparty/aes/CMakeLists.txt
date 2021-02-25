cmake_minimum_required(VERSION 3.2)
project(aes)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(aes STATIC ${SOURCES})

add_library(mdv::aes ALIAS aes)

target_include_directories(aes PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
