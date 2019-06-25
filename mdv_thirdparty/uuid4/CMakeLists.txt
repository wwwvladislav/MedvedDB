cmake_minimum_required(VERSION 3.2)
project(uuid4)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c) 

use_c11()

include_directories(
    ./
)

add_library(uuid4 STATIC ${SOURCES})
