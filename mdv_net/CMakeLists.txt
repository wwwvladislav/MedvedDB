cmake_minimum_required(VERSION 3.2)
project(mdv_net)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

find_package(Threads REQUIRED)

add_library(mdv_net STATIC ${SOURCES})

add_library(mdv::net ALIAS mdv_net)

target_include_directories(mdv_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mdv_net PUBLIC mdv::platform mdv::zf_log
                              PRIVATE Threads::Threads)
