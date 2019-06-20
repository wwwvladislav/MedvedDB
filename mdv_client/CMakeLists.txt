cmake_minimum_required(VERSION 3.4)
project(mdv)

add_definitions(-Wall -Wextra)

set(HEADERS
)

set(SOURCES
    main.c
)

include_directories(
    ${LIBS_DIR}/binn
    ${ROOT_DIR}/mdv_api
    ${ROOT_DIR}/mdv_platform
)

use_c11()

add_executable(mdv ${HEADERS} ${SOURCES})
add_dependencies(mdv mdv_api mdv_platform)
target_link_libraries(mdv mdv_api mdv_platform)
