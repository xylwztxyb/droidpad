
aux_source_directory(${PROJECT_SOURCE_DIR}/src/main/jni JNI_SOURCE_FILES)
add_library(droidpad
        SHARED
        ${JNI_SOURCE_FILES})

add_dependencies(droidpad zinnia caffe)

target_include_directories(droidpad
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/main/cpp/zinnia/include
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/external/opencv/include
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/external/boost/include
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/external/protobuf/include
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/external/gflags/include
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/external/openblas/include
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/include
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/external/glog/include
        ${PROJECT_BINARY_DIR}/include)

target_link_libraries(droidpad
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libzinnia.so
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcaffe.so
        ${PROJECT_SOURCE_DIR}/src/main/cpp/caffe/external/opencv/lib/${ANDROID_ABI}/libopencv_core.a
        log)