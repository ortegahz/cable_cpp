/*
 * @Author: wx
 * @Date: 2024-08-05 15:28:25
 * @LastEditors: wx
 * @LastEditTime: 2024-08-05 15:28:25
 * @FilePath: alg_cable_tem_detector.h
 * @Description:
 */

#ifndef __ALG_CABLE_TEMPERATURE_DETECTOR_H__
#define __ALG_CABLE_TEMPERATURE_DETECTOR_H__

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

// #define CABLE_TEMP_DEBUG 1
// #if defined(_WIN32)
// #if CABLE_TEMP_DEBUG
// #define LOGI(fmt, ...) printf("wx_test: line %d, " fmt"\n", __LINE__,##__VA_ARGS__)
// #else
// #define LOGI(fmt, args...)
// #endif
// #endif

#define MAX_LENGTH 64
#define ONE_GROUP_DATA_LENGTH 64
#define CONSTANT_TEMPERATUE_THRESHOLD 100

// cable temperature detector
class CableTemDet
{
public:
    CableTemDet();
    ~CableTemDet();
    int init(int _control);
    int run(int *_data, int _idx, int _cable_idx);

private:
    bool alarm_gb_switch_flag = true;
    bool alarm_temdiff_switch_flag = true;
    bool alarm_shape_switch_flag = true;
    bool background_temperatures_confirm_flag = false;
    int background_temperatures_confirm_matrix = 0;
    float background_temperatures[MAX_LENGTH] = {};
    int updateBackgroundTemperature(int *_data, int _idx);
    int calSbtractBackground(float *_sbbg, int *_data, int _idx);
};


#endif // end define __ALG_CABLE_TEMPERATURE_DETECTOR_H__