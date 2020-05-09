cmake_minimum_required(VERSION 3.2)
project(mdv_bindings)

find_package(SWIG)
find_package(JNI)
find_package(Java COMPONENTS Runtime Development)

add_subdirectory(mdv)
add_subdirectory(mdb)
