
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CURRENT_DIR "${PROJECT_SOURCE_DIR}/src/main/cpp/caffe")

set(CAFFE_TARGET_VERSION "1.0.0" CACHE STRING "Caffe logical version")
set(CAFFE_TARGET_SOVERSION "1.0.0" CACHE STRING "Caffe soname version")
add_definitions(-DCAFFE_VERSION=${CAFFE_TARGET_VERSION})

add_definitions(-DUSE_OPENCV)

#Set protobuf files
find_program(PROTOC_BIN protoc /usr/bin /usr/local/bin)
find_file(CAFFE_PROTO_FILE caffe.proto "${CURRENT_DIR}/proto")
exec_program(mkdir ARGS "-p ${PROJECT_BINARY_DIR}/include/caffe/proto")
exec_program(${PROTOC_BIN} ARGS "--cpp_out=${PROJECT_BINARY_DIR}/include/caffe/proto \
             -I${CURRENT_DIR}/proto \
            ${CAFFE_PROTO_FILE}")

set(EXTERN_LIB_DIR ${CURRENT_DIR}/external)
#Set openblas
link_libraries(${EXTERN_LIB_DIR}/openblas/lib/${ANDROID_ABI}/libopenblas.a)

#Set glog
link_libraries(${EXTERN_LIB_DIR}/glog/lib/${ANDROID_ABI}/libglog.a)

#Set gflags
link_libraries(${EXTERN_LIB_DIR}/gflags/lib/${ANDROID_ABI}/libgflags.a)

#set boost
set(_boost_lib "boost_system" "boost_filesystem" "boost_thread")
foreach (lib_name ${_boost_lib})
    link_libraries(${EXTERN_LIB_DIR}/boost/lib/${ANDROID_ABI}/lib${lib_name}.a)
endforeach ()
unset(lib_name CACHE)
unset(_boost_lib CACHE)

#set protobuf
link_libraries(${EXTERN_LIB_DIR}/protobuf/lib/${ANDROID_ABI}/libprotobuf.a)

#set opencv
set(_opencv_lib "opencv_highgui" "opencv_imgproc" "opencv_core")
foreach (lib_name ${_opencv_lib})
    link_libraries(${EXTERN_LIB_DIR}/opencv/lib/${ANDROID_ABI}/lib${lib_name}.a)
endforeach ()
unset(lib_name CACHE)
unset(_opencv_lib CACHE)

#set opencv 3rd libs
set(_opencv_3rd_lib "libjasper" "libjpeg" "libpng" "libtiff")
foreach (lib_name ${_opencv_3rd_lib})
    link_libraries(${EXTERN_LIB_DIR}/opencv/3rdparty/libs/${ANDROID_ABI}/lib${lib_name}.a)
endforeach ()
unset(lib_name CACHE)
unset(_opencv_3rd_lib CACHE)

aux_source_directory(${CURRENT_DIR} CAFFE_SOURCE_FILES)
aux_source_directory(${CURRENT_DIR}/layers CAFFE_SOURCE_FILES)
aux_source_directory(${CURRENT_DIR}/util CAFFE_SOURCE_FILES)
aux_source_directory(${PROJECT_BINARY_DIR}/include/caffe/proto CAFFE_SOURCE_FILES)

include_directories(${CURRENT_DIR}/include)
add_library(caffe SHARED ${CAFFE_SOURCE_FILES})

target_include_directories(caffe
        PRIVATE
        ${PROJECT_BINARY_DIR}/include
        ${EXTERN_LIB_DIR}/openblas/include
        ${EXTERN_LIB_DIR}/glog/include
        ${EXTERN_LIB_DIR}/gflags/include
        ${EXTERN_LIB_DIR}/boost/include
        ${EXTERN_LIB_DIR}/protobuf/include
        ${EXTERN_LIB_DIR}/opencv/include

        )

target_compile_options(caffe PRIVATE "-stdlib=libstdc++"
        "-Wno-sign-compare"
        "-Wno-uninitialized"
        )
target_link_libraries(caffe "z" "log")


  








