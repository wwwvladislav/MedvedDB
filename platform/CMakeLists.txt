cmake_minimum_required(VERSION 3.2)
project(platform)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

if (${PLATFORM} STREQUAL LINUX)
    file(GLOB LINUX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/linux/*.c)
    set(SOURCES ${SOURCES} ${LINUX_SOURCES})
else()
    # TODO
endif()

use_c11()

include_directories(
    ${LIBS_DIR}/zf_log
)

add_library(platform STATIC ${SOURCES})
