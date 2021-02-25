cmake_minimum_required(VERSION 3.2)
project(inih)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c) 

use_c11()

add_library(inih STATIC ${SOURCES})

add_library(mdv::inih ALIAS inih)

target_include_directories(inih PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
