cmake_minimum_required(VERSION 3.2)
project(mdv_storage)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

add_library(mdv_storage STATIC ${SOURCES})

add_library(mdv::storage ALIAS mdv_storage)

target_include_directories(mdv_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mdv_storage PUBLIC mdv::platform mdv::zf_log mdv::rpmalloc mdv::uuid4)
