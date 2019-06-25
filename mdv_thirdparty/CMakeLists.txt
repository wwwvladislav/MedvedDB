
add_subdirectory(inih)

add_subdirectory(zf_log)

add_subdirectory(lmdb)

set(NNG_TESTS OFF CACHE BOOL "Build and run tests")
set(NNG_TOOLS OFF CACHE BOOL "Build extra tools")
add_subdirectory(nng)

add_subdirectory(binn)

add_subdirectory(rpmalloc)

add_subdirectory(uuid4)
