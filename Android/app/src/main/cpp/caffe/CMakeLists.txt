

SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)

set(CURRENT_DIR "${PROJECT_SOURCE_DIR}/src/main/cpp/caffe")

set(CAFFE_TARGET_VERSION "1.0.0" CACHE STRING "Caffe logical version")
set(CAFFE_TARGET_SOVERSION "1.0.0" CACHE STRING "Caffe soname version")
add_definitions(-DCAFFE_VERSION=${CAFFE_TARGET_VERSION})

#option(USE_OPENCV "Build with Opencv Support" ON)
#list(APPEND CMAKE_CXX_FLAGS "-stdlib=libstdc++")
#list(APPEND CMAKE_CXX_FLAGS "-Wno-sign-compare -Wno-uninitialized")
#list(APPEND CMAKE_CXX_FLAGS "-DUSE_OPENCV")
add_definitions(-DUSE_OPENCV)

#configure_file(cmake/caffe_config.h.in "${PROJECT_BINARY_DIR}/caffe_config.h")

#include_directories(${PROJECT_BINARY_DIR})
#Set protobuf files
find_program(PROTOC_BIN protoc /usr/bin /usr/local/bin)
find_file(CAFFE_PROTO_FILE caffe.proto "${CURRENT_DIR}/proto")
exec_program(mkdir ARGS "-p ${PROJECT_BINARY_DIR}/include/caffe/proto")
exec_program(${PROTOC_BIN} ARGS "--cpp_out=${PROJECT_BINARY_DIR}/include/caffe/proto \
             -I${CURRENT_DIR}/proto \
            ${CAFFE_PROTO_FILE}")
#include_directories(${PROJECT_BINARY_DIR}/include)

set(EXTERN_LIB_DIR ${CURRENT_DIR}/external)
#Set openblas
#include_directories(${EXTERN_LIB_DIR}/openblas/include)
#list(APPEND CMAKE_FIND_ROOT_PATH "${EXTERN_LIB_DIR}/openblas/lib/${ANDROID_ABI}")
find_library(OPENBLAS_LIBS libopenblas.a ${EXTERN_LIB_DIR}/openblas/lib/${ANDROID_ABI})

#Set glog
#include_directories(${EXTERN_LIB_DIR}/glog/include)
find_library(GLOG_LIBS libglog.a ${EXTERN_LIB_DIR}/glog/lib/${ANDROID_ABI})

#Set gflags
#include_directories(${EXTERN_LIB_DIR}/gflags/include)
find_library(GFLAGS_LIBS libgflags.a ${EXTERN_LIB_DIR}/gflags/lib/${ANDROID_ABI})

#set boost
#include_directories(${EXTERN_LIB_DIR}/boost/include)
set(BOOST_LIBS "")
set(_boost_lib "boost_system" "boost_filesystem" "boost_thread")
foreach(lib ${_boost_lib})
  find_library(lib_path lib${lib}.a ${EXTERN_LIB_DIR}/boost/lib/${ANDROID_ABI})
  list(APPEND BOOST_LIBS ${lib_path})
  unset(lib_path CACHE)
endforeach()
unset(lib CACHE)
unset(_boost_lib CACHE)

#set protobuf
#include_directories(${EXTERN_LIB_DIR}/protobuf/include)
find_library(PROTOBUF_LIBS libprotobuf.a ${EXTERN_LIB_DIR}/protobuf/lib/${ANDROID_ABI})

#set opencv
#include_directories(${EXTERN_LIB_DIR}/opencv/include)
set(OPENCV_LIBS "")
set(_opencv_lib "opencv_highgui" "opencv_imgproc" "opencv_core")
foreach(lib ${_opencv_lib})
  find_library(lib_path lib${lib}.a ${EXTERN_LIB_DIR}/opencv/lib/${ANDROID_ABI})
  list(APPEND OPENCV_LIBS ${lib_path})
  unset(lib_path CACHE)
endforeach()
unset(lib CACHE)
unset(_opencv_lib CACHE)

#set opencv 3rd libs
set(_opencv_3rd_lib "libjasper" "libjpeg" "libpng" "libtiff")
foreach(lib ${_opencv_3rd_lib})
  find_library(lib_path lib${lib}.a ${EXTERN_LIB_DIR}/opencv/3rdparty/libs/${ANDROID_ABI})
  list(APPEND OPENCV_LIBS ${lib_path})
  unset(lib_path CACHE)
endforeach()
unset(lib CACHE)
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
target_link_libraries(caffe ${OPENBLAS_LIBS} ${BOOST_LIBS} ${PROTOBUF_LIBS} ${OPENCV_LIBS}
		     ${GLOG_LIBS} ${GFLAGS_LIBS} "z" "log")


  








