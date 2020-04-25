cmake_minimum_required(VERSION 3.2)
project(mdv_storage)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${ROOT_DIR}/mdv_platform
    ${LIBS_DIR}/zf_log
    ${LIBS_DIR}/rpmalloc
    ${LIBS_DIR}/uuid4
)

add_library(mdv_storage STATIC ${SOURCES})

add_dependencies(mdv_storage zf_log rpmalloc uuid4)

target_link_libraries(mdv_storage zf_log rpmalloc uuid4)
