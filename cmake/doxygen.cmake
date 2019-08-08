

macro(configure_doxyfile DOXYGEN_CONFIG_FILE FILE_NAME_SUFFIX)

    if(EXISTS ${PROJECT_SOURCE_DIR}/${DOXYGEN_CONFIG_FILE})

        file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/doxy-${FILE_NAME_SUFFIX}.conf)
        file(STRINGS ${PROJECT_SOURCE_DIR}/${DOXYGEN_CONFIG_FILE} DOXYFILE_LINES)

        list(LENGTH DOXYFILE_LINES ROW)

        math(EXPR ROW "${ROW} - 1")

        foreach(I RANGE ${ROW})

            list(GET DOXYFILE_LINES ${I} LINE)

            string(REPLACE ";" " " LINE "${LINE}")

            if(LINE STRGREATER "")
                string(REGEX MATCH "^[a-zA-Z]([^ ])+" DOXY_PARAM ${LINE})

                if(DEFINED DOXY_${DOXY_PARAM})
                    string(REGEX REPLACE "=([^\n])+" "= ${DOXY_${DOXY_PARAM}}" LINE ${LINE})
                    string(REPLACE ";" " " LINE "${LINE}")
                endif()
            endif()

            file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/doxy-${FILE_NAME_SUFFIX}.conf "${LINE}\n")

        endforeach()
    else()
        message(SEND_ERROR "Doxygen configuration file '${DOXYGEN_CONFIG_FILE}' not found. Documentation will not be generated")
    endif()

endmacro(configure_doxyfile)


macro(add_documentation TARGET DOXYGEN_CONFIG_FILE)

    find_package(Doxygen)

    if(DOXYGEN_FOUND)
        configure_doxyfile(${DOXYGEN_CONFIG_FILE} ${TARGET})
        add_custom_target(${TARGET} COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxy-${TARGET}.conf)
    else()
        message(STATUS "Doxygen not found. Documentation will not be generated")
    endif()

endmacro(add_documentation)

