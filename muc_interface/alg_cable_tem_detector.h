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

#define CABLE_TEMP_DEBUG 1
#if defined(_WIN32)
#if CABLE_TEMP_DEBUG
#define LOGI(fmt, ...) printf("wx_test: line %d, " fmt"\n", __LINE__,##__VA_ARGS__)
#else
#define LOGI(fmt, args...)
#endif
#endif

#define MAX_LENGTH 256
#define ONE_GROUP_DATA_LENGTH 64
#define CONSTANT_TEMPERATUE_THRESHOLD 100
#define BEAR_TIME_THRESHOLD 10
#define ACC_TIME_THRESHOLD 32
#define SAVE_ARCHS_MAX_NUM_PER_PEAK 63

#define FIND_PEAK_WINDOW_SIZE 16
#define FIND_PEAK_TEMP_TH  2.5

// arch struct
typedef struct ArchInfo
{
    int peak;
    float peak_val;
    int p_start;
    int p_end;
    int arch_trend;
    int timestamp;
}ArchInfo;
typedef struct ArchInfoArr
{
    int arch_id;
    std::vector<ArchInfo> archs;
}ArchInfoArr;
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
    float m_background_temperatures[MAX_LENGTH] = {};
    float m_current_temperatures[MAX_LENGTH] = {};
    int updateBackgroundTemperature(int *_data, int _idx);
    int calSbtractBackground(float *_sbbg, int *_data, int _idx);
    int m_timestamp = 0;
    std::vector<ArchInfoArr> m_analyse_window;
    int trackArch(ArchInfo &_arch, int _peak_win);
    int detactArch(float *_arr, std::vector<int> _peak_idx, int _MAX_LENGTH, int _peak_win);
    int deleteOldArchData();

    int alarmShape(int _arch_trend_th, float _reliable_arch_ratio_th);
    int alarmTemperatureRise(float _temperature_rise_thre);
};


#endif // end define __ALG_CABLE_TEMPERATURE_DETECTOR_H__