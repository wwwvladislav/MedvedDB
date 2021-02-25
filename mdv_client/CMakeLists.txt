cmake_minimum_required(VERSION 3.4)
project(mdv)

set(HEADERS
    mdv_cout.h
)

set(SOURCES
    main.c
    mdv_cout.c
)

use_c11()

add_executable(mdv ${HEADERS} ${SOURCES})

target_compile_options(mdv PRIVATE -Wall -Wextra)

target_link_libraries(mdv PRIVATE mdv::api mdv::linenoise)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ./
        LIBRARY DESTINATION lib
)
