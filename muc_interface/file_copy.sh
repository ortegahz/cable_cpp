#!/bin/bash

# 定义要拷贝的文件列表
files=("ai_shape_inference.h" "ai_shape_inference_mcu.cpp" "alg_cable_tem_detector.h" "alg_cable_tem_detector.cpp" "alg_cable_tem_detector_interface.cpp" "alg_cable_tem_detector_interface.h")

# 定义目标文件夹
destination="/media/manu/ST2000DM005-2U91/workspace/temperature-sensitive/Project/Algorithm/"

# 拷贝每个文件到目标文件夹
for file in "${files[@]}"; do
    cp "$file" "$destination"
    if [[ $? -eq 0 ]]; then
        echo "Successfully copied $file to $destination"
    else
        echo "Failed to copy $file to $destination"
    fi
done
