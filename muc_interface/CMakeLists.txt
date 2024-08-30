cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 11)

project(AICableTemperarueDetector)

add_executable(pc
    main.cpp
    alg_cable_tem_detector_interface.cpp
    alg_cable_tem_detector.cpp)

# find tflite
set(TFLITE_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/tflite-2.3.1-lib")
include_directories("${TFLITE_LIBRARY_DIR}/include")
add_library( tflite  SHARED IMPORTED )
set_target_properties(
        tflite
        PROPERTIES IMPORTED_LOCATION
        "${TFLITE_LIBRARY_DIR}/lib/tensorflowlite_c.dll"
)

set(EXCUtTE_NAME main_pc_tflite_debug)
add_executable(${EXCUtTE_NAME}
        main_pc_tflite_debug.cpp
        ai_shape_inference_pc.cpp)
target_link_libraries(${EXCUtTE_NAME} ${TFLITE_LIBRARY_DIR}/lib/tensorflowlite_c.dll)

# 这个可执行程序是AI感温线缆的主程序
set(EXCUtTE_NAME mcu)
add_executable(${EXCUtTE_NAME}
        main_mcu.cpp
        alg_cable_tem_detector_interface.cpp
        alg_cable_tem_detector.cpp
        ai_shape_inference_pc.cpp)
target_link_libraries(${EXCUtTE_NAME} ${TFLITE_LIBRARY_DIR}/lib/tensorflowlite_c.dll)
