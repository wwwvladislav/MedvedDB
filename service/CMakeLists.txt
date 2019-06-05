cmake_minimum_required(VERSION 3.4)
project(medved)

add_definitions(-Wall -Werror -Wextra)

set(HEADERS
)

set(SOURCES
    main.c
)

include_directories(
    ${ROOT_DIR}/platform
    ${ROOT_DIR}/core
    ${LIBS_DIR}/zf_log
)

use_c11()

add_executable(medved ${HEADERS} ${SOURCES})
add_dependencies(medved platform core)
target_link_libraries(medved platform core)
