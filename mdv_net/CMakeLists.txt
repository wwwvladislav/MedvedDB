cmake_minimum_required(VERSION 3.2)
project(mdv_net)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${ROOT_DIR}/mdv_platform
    ${LIBS_DIR}/zf_log
    ${LIBS_DIR}/rpmalloc
)

find_package(Threads REQUIRED)

add_library(mdv_net STATIC ${SOURCES})

target_link_libraries(mdv_net mdv_platform zf_log rpmalloc Threads::Threads)
