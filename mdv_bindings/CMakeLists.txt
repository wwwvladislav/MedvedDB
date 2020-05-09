cmake_minimum_required(VERSION 3.2)
project(mdv_bindings)

set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(JAVA_INCLUDE_PATH2 NotNeeded)
endif()

find_package(JNI)
find_package(SWIG)
find_package(Java COMPONENTS Runtime Development)

add_subdirectory(mdv)
add_subdirectory(mdb)
