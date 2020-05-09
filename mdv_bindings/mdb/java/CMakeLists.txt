cmake_minimum_required(VERSION 3.2)
project(mdb4j)

use_c11()

if(BUILD_JNI)

    if(JNI_FOUND)

        set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mdv_svc4j.c)

        if(SWIG_FOUND)
            set(SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})
            set(SWIG_OUT ${SOURCES})

            set(SWIG_FLAGS
                -java
                -outdir     ${SWIG_OUTDIR}
                -o          ${SWIG_OUT}
                -package    mdv
                -I${ROOT_DIR}/mdv_api
                -I${ROOT_DIR}/mdv_types
                -I${ROOT_DIR}/mdv_platform
                -I${ROOT_DIR}/mdv_core
            )

            set(SWIG_FILE ${ROOT_DIR}/assets/swig/mdb/mdb.i)

            set(JAVA_SOURCES
                ${SWIG_OUTDIR}/mdv.java
                ${SWIG_OUTDIR}/mdvJNI.java
                ${SWIG_OUTDIR}/Service.java
            )

            add_custom_target(mdb_swig4j ALL
                COMMAND     ${SWIG_EXECUTABLE} ${SWIG_FLAGS} ${SWIG_FILE}
                BYPRODUCTS  ${SWIG_OUT} ${JAVA_SOURCES})

            if(Java_FOUND)
                include(UseJava)

                set(CMAKE_JNI_TARGET TRUE)

                add_jar(mdb_java ${JAVA_SOURCES}
                        VERSION 1.0.0
                        OUTPUT_NAME mdb4j
                        OUTPUT_DIR ${SWIG_OUTDIR})

                add_dependencies(mdb_java mdb_swig4j)
            endif(Java_FOUND)

        endif(SWIG_FOUND)

        include_directories(
            ${ROOT_DIR}/mdv_api
            ${ROOT_DIR}/mdv_types
            ${ROOT_DIR}/mdv_platform
            ${ROOT_DIR}/mdv_core
            ${JNI_INCLUDE_DIRS})

        add_library(mdb4j SHARED ${SOURCES})

        if(SWIG_FOUND)
            add_dependencies(mdb4j mdv_core mdb_swig4j)
        else()
            add_dependencies(mdb4j mdv_core)
        endif()

        target_link_libraries(mdb4j mdv_core)

        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ./
            LIBRARY DESTINATION lib
        )
    else(JNI_FOUND)
        message("JNI package not found. JNI library building is skipped.")
    endif(JNI_FOUND)
endif(BUILD_JNI)
