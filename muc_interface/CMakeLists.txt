cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 11)

project(AICableTemperarueDetector)

add_executable(pc
    main.cpp
    alg_cable_tem_detector_interface.cpp
    alg_cable_tem_detector.cpp)

add_executable(mcu
        main_mcu.cpp
        alg_cable_tem_detector_interface.cpp
        alg_cable_tem_detector.cpp)

