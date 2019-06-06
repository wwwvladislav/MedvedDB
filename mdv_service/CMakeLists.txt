cmake_minimum_required(VERSION 3.4)
project(medved)

add_definitions(-Wall -Werror -Wextra)

set(HEADERS
)

set(SOURCES
    main.c
)

include_directories(
    ${ROOT_DIR}/mdv_platform
    ${ROOT_DIR}/mdv_core
    ${LIBS_DIR}/zf_log
)

use_c11()

add_executable(medved ${HEADERS} ${SOURCES})
add_dependencies(medved mdv_platform mdv_core)
target_link_libraries(medved mdv_platform mdv_core)
