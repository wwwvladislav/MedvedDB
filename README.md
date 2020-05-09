[![ci][badge.ci]][ci]

[badge.ci]: https://github.com/wwwVladislav/MedvedDB/workflows/C/C++%20CI/badge.svg?branch=master

[ci]: https://github.com/wwwVladislav/MedvedDB/actions

# MedvedDB
MedvedDB is NoSQL distributed database.
The main properties are following:
1. High availability. Fully decentralized and doesn't have any points of failure.
2. Support eventual consistency model.
3. Fully transactional and complies with the ACID properties. This properties are inherited from LMDB database. LMDB database is used as the low level storage.

### Supported platforms:
1. Linux (Ubuntu)
2. Android

### Supported language bindings
MedvedDB API can be used from other programming languages. API for specific language is described via SWIG.
 * Java \
Java API generation requires SWIG and JDK packages. Then simply turn on the BUILD_JNI option in CMakeLists.txt 
and build the project. The JNI library and Java JAR file will be placed in the build/mdv_bindings/mdv/java folder 
(search the libmdv4j.so and mdv4j.jar files).

### Building MedvedDB from Source
```
mkdir build
cd build
cmake ..
cmake --build .
```
