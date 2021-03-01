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

add_library(mdv_core STATIC ${SOURCES})

add_library(mdv::core ALIAS mdv_core)

target_include_directories(mdv_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(mdv_core PUBLIC
    mdv::inih
    mdv::zf_log
    mdv::platform
    mdv::net
    mdv::api
    mdv::storage
)
