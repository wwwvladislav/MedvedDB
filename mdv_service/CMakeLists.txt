cmake_minimum_required(VERSION 3.4)
project(medved)

set(HEADERS
)

set(SOURCES
    main.c
)

use_c11()

add_executable(medved ${HEADERS} ${SOURCES})

target_compile_options(medved PRIVATE -Wall -Werror -Wextra)

target_link_libraries(medved PRIVATE mdv::api mdv::core mdv::zf_log)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION ./
        LIBRARY DESTINATION lib
)
