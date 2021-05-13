cmake_minimum_required(VERSION 3.2)
project(mdv_platform)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

find_package(Threads REQUIRED)

add_library(mdv_platform STATIC ${SOURCES})

add_library(mdv::platform ALIAS mdv_platform)

target_include_directories(mdv_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/generic)

target_link_libraries(mdv_platform
                      PUBLIC mdv::zf_log mdv::binn mdv::uuid4
                      PRIVATE Threads::Threads m)
