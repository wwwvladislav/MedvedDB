cmake_minimum_required(VERSION 3.2)
project(mdb4j)

use_c11()

if(BUILD_JNI)

    if(JNI_FOUND)

        set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mdb_svc4j.c)

        if(SWIG_FOUND)
            set(SWIG_OUTDIR ${CMAKE_CURRENT_BINARY_DIR})
            set(SWIG_OUT ${SOURCES})

            set(SWIG_FLAGS
                -java
                -outdir     ${SWIG_OUTDIR}
                -o          ${SWIG_OUT}
                -package    mdb
                -I${ROOT_DIR}/mdv_api
                -I${ROOT_DIR}/mdv_types
                -I${ROOT_DIR}/mdv_platform
                -I${ROOT_DIR}/mdv_core
            )

            set(SWIG_FILE ${ROOT_DIR}/assets/swig/mdb/mdb.i)

            set(JAVA_SOURCES
                ${SWIG_OUTDIR}/mdb.java
                ${SWIG_OUTDIR}/mdbJNI.java
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

        add_library(mdb4j SHARED ${SOURCES})

        add_library(mdv::mdb4j ALIAS mdb4j)

        target_include_directories(mdb4j PRIVATE ${JNI_INCLUDE_DIRS})

        if(SWIG_FOUND)
            add_dependencies(mdb4j mdb_swig4j)
        endif()

        target_link_libraries(mdb4j mdv::core)

        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ./
            LIBRARY DESTINATION lib
        )
    else(JNI_FOUND)
        message("JNI package not found. JNI library building is skipped.")
    endif(JNI_FOUND)
endif(BUILD_JNI)
