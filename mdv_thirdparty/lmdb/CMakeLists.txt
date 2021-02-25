cmake_minimum_required(VERSION 3.2)
project(lmdb)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

find_package(Threads REQUIRED)

add_library(lmdb STATIC ${SOURCES})

add_library(mdv::lmdb ALIAS lmdb)

target_compile_definitions(lmdb PUBLIC MDB_VL32)

target_link_libraries(lmdb PRIVATE Threads::Threads)

target_include_directories(lmdb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
