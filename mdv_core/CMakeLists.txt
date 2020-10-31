cmake_minimum_required(VERSION 3.2)
project(mdv_core)

use_c11()

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

find_package(FLEX)
find_package(BISON)

if(FLEX_FOUND AND BISON_FOUND)
    flex_target(SQL_LEXER
        ${CMAKE_CURRENT_SOURCE_DIR}/storage/mdv_sql.l
        ${CMAKE_CURRENT_SOURCE_DIR}/storage/mdv_sql_parser.c)
    bison_target(SQL_PARSER
        ${CMAKE_CURRENT_SOURCE_DIR}/storage/mdv_sql.y
        ${CMAKE_CURRENT_SOURCE_DIR}/storage/mdv_sql_lexer.c)
    add_flex_bison_dependency(SQL_LEXER SQL_PARSER)

    set(SOURCES ${SOURCES}
        ${FLEX_SQL_LEXER_OUTPUTS}
        ${BISON_SQL_PARSER_OUTPUTS}
    )
endif()

include_directories(
    ${LIBS_DIR}/binn
    ${LIBS_DIR}/inih
    ${LIBS_DIR}/zf_log
    ${LIBS_DIR}/lmdb
    ${ROOT_DIR}/mdv_platform
    ${ROOT_DIR}/mdv_net
    ${ROOT_DIR}/mdv_api
    ${ROOT_DIR}/mdv_types
)

add_library(mdv_core STATIC ${SOURCES})

target_link_libraries(mdv_core inih zf_log mdv_platform mdv_net mdv_api mdv_types lmdb)
