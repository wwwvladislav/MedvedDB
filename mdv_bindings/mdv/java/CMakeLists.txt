cmake_minimum_required(VERSION 3.2)
project(mdv4j)

use_c11()

if(BUILD_JNI)

    if(JNI_FOUND)

        set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/mdv_api4j.c)

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
            )

            set(SWIG_FILE ${ROOT_DIR}/assets/swig/mdv/mdv.i)

            set(JAVA_SOURCES
                ${SWIG_OUTDIR}/mdv.java
                ${SWIG_OUTDIR}/mdvJNI.java
                ${SWIG_OUTDIR}/UUID.java
                ${SWIG_OUTDIR}/BitSet.java
                ${SWIG_OUTDIR}/Field.java
                ${SWIG_OUTDIR}/FieldType.java
                ${SWIG_OUTDIR}/TableDesc.java
                ${SWIG_OUTDIR}/Table.java
                ${SWIG_OUTDIR}/Row.java
                ${SWIG_OUTDIR}/RowSet.java
                ${SWIG_OUTDIR}/RowSetEnumerator.java
                ${SWIG_OUTDIR}/ArrayOfBool.java
                ${SWIG_OUTDIR}/ArrayOfChar.java
                ${SWIG_OUTDIR}/ArrayOfByte.java
                ${SWIG_OUTDIR}/ArrayOfUint8.java
                ${SWIG_OUTDIR}/ArrayOfInt8.java
                ${SWIG_OUTDIR}/ArrayOfUint16.java
                ${SWIG_OUTDIR}/ArrayOfInt16.java
                ${SWIG_OUTDIR}/ArrayOfUint32.java
                ${SWIG_OUTDIR}/ArrayOfInt32.java
                ${SWIG_OUTDIR}/ArrayOfUint64.java
                ${SWIG_OUTDIR}/ArrayOfInt64.java
                ${SWIG_OUTDIR}/ArrayOfFloat.java
                ${SWIG_OUTDIR}/ArrayOfDouble.java
                ${SWIG_OUTDIR}/Client.java
                ${SWIG_OUTDIR}/ClientConfig.java
                ${SWIG_OUTDIR}/ClientDbConfig.java
                ${SWIG_OUTDIR}/ClientConnectionConfig.java
                ${SWIG_OUTDIR}/ClientThreadpoolConfig.java
            )

            add_custom_target(mdv_swig4j ALL
                COMMAND     ${SWIG_EXECUTABLE} ${SWIG_FLAGS} ${SWIG_FILE}
                BYPRODUCTS  ${SWIG_OUT} ${JAVA_SOURCES})

            if(Java_FOUND)
                include(UseJava)

                set(CMAKE_JNI_TARGET TRUE)

                add_jar(mdv_java ${JAVA_SOURCES}
                        VERSION 1.0.0
                        OUTPUT_NAME mdv4j
                        OUTPUT_DIR ${SWIG_OUTDIR})

                add_dependencies(mdv_java mdv_swig4j)
            endif(Java_FOUND)

        endif(SWIG_FOUND)

        include_directories(
            ${ROOT_DIR}/mdv_api
            ${ROOT_DIR}/mdv_types
            ${ROOT_DIR}/mdv_platform
            ${JNI_INCLUDE_DIRS})

        add_library(mdv4j SHARED ${SOURCES})

        if(SWIG_FOUND)
            add_dependencies(mdv4j mdv_api mdv_swig4j)
        else()
            add_dependencies(mdv4j mdv_api)
        endif()

        target_link_libraries(mdv4j mdv_api)

        install(TARGETS ${PROJECT_NAME}
            RUNTIME DESTINATION ./
            LIBRARY DESTINATION lib
        )
    else(JNI_FOUND)
        message("JNI package not found. JNI library building is skipped.")
    endif(JNI_FOUND)
endif(BUILD_JNI)
