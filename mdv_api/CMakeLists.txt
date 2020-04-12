cmake_minimum_required(VERSION 3.2)
project(mdv_api)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

use_c11()

include_directories(
    ${LIBS_DIR}/binn
    ${ROOT_DIR}/mdv_platform
    ${ROOT_DIR}/mdv_net
    ${ROOT_DIR}/mdv_types
)

add_library(mdv_api SHARED ${SOURCES})

add_dependencies(mdv_api mdv_platform mdv_net mdv_types)

target_link_libraries(mdv_api mdv_platform mdv_net mdv_types)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ./
        LIBRARY DESTINATION lib
)
