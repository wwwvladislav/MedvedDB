cmake_minimum_required(VERSION 3.4)
project(mdv)

add_definitions(-Wall -Wextra)

set(HEADERS
    mdv_cout.h
)

set(SOURCES
    main.c
    mdv_cout.c
)

include_directories(
    ${LIBS_DIR}/binn
    ${LIBS_DIR}/linenoise
    ${ROOT_DIR}/mdv_api
    ${ROOT_DIR}/mdv_platform
    ${ROOT_DIR}/mdv_types
)

use_c11()

add_executable(mdv ${HEADERS} ${SOURCES})

add_dependencies(mdv mdv_api mdv_platform mdv_types linenoise)

target_link_libraries(mdv mdv_api mdv_platform mdv_types linenoise)
