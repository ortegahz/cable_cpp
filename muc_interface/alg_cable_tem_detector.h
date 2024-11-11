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
#include <stdint.h>
#if defined(__arm__)
#include <rtthread.h>
#endif
#include "ai_shape_inference.h"

#define CABLE_TEMP_DEBUG 1
#if CABLE_TEMP_DEBUG
#if defined(_WIN32) || defined(__linux__)
#define LOGI(fmt, ...) printf("wx_test: line %d, " fmt, __LINE__, ##__VA_ARGS__)
#define LOGD(fmt, ...) printf("wx_test: line %d, " fmt, __LINE__, ##__VA_ARGS__)
#elif defined(__arm__)
#define LOGI(fmt, ...) rt_kprintf("line %d, " fmt, __LINE__,##__VA_ARGS__)
#define LOGD(fmt, ...) rt_kprintf("line %d, " fmt, __LINE__,##__VA_ARGS__)
#endif
#else
#if defined(_WIN32) || defined(__linux__)
#define LOGI(fmt, ...) printf(fmt,  ##__VA_ARGS__)
#define LOGD(fmt, ...)
#elif defined(__arm__)
#define LOGI(fmt, ...) rt_kprintf(fmt,  ##__VA_ARGS__)
#define LOGD(fmt, ...)
#endif
#endif

#define MAX_LENGTH 1024
#define ONE_GROUP_DATA_LENGTH 64

#define TIME_INIT_CNT_TH 16
#define TIME_RUN_CNT_TH 64
#define BEAR_TIME_THRESHOLD 10
#define ACC_TIME_THRESHOLD 16
#define SAVE_ARCHS_MAX_NUM_PER_PEAK 32

#define FIND_PEAK_WINDOW_SIZE 16
// SAVE_ACTULLY_BUFFER_SIZE = FIND_PEAK_WINDOW_SIZE * 2
#define SAVE_ACTULLY_BUFFER_SIZE 32
#define FIND_PEAK_TEMP_TH  2.5
#define ANALYSE_WINDOW_MAX  16

#define ALARM_CONSTANT_TEMPERATUE_THRESHOLD 100
#define ALARM_ARCH_TREND_TH  4
#define ALARM_ARCH_RATIO_TH  0.8
#define ALARM_TEMPERATURE_RISE_THRESHLOD 8.0

#define SHAPE_MODEL_PROBABILITY_BUFFER_SIZE 5
#define SHAPE_MODEL_AVERAGE_PROB_TH 0.7

typedef struct PeakInfo{
    int index;
    float value;
} PeakInfo;

typedef struct ArchInfo
{
    int peak;
    float peak_val;
    int p_start;
    int p_end;
    int arch_trend;
    int timestamp;
    // 储存实际采集到的温度
    signed char temp[SAVE_ACTULLY_BUFFER_SIZE] = {0};
    signed char temp_sub[SAVE_ACTULLY_BUFFER_SIZE] = {0};
} ArchInfo;

typedef struct ArchInfoArr
{
    int arch_id;
    ArchInfo archs[SAVE_ARCHS_MAX_NUM_PER_PEAK + 1];
    int arch_count;
    float shape_prob_buffer[SHAPE_MODEL_PROBABILITY_BUFFER_SIZE] = {0.0f};
} ArchInfoArr;

// cable temperature detector
class CableTemDet
{
public:
    CableTemDet();
    ~CableTemDet();
    int init(int _control);
    int run(int *_data, int _idx, int _cable_idx, uint32_t _timestamp, int8_t _use_ai_model);
    int8_t m_alarm_status[MAX_LENGTH];
    int8_t m_alarm_val[MAX_LENGTH];

private:
    bool m_alarm_suppression = false;
    int m_idx;
    bool m_init_flag = false;
    bool alarm_gb_switch_flag = true;
    bool alarm_temdiff_switch_flag = true;
    bool alarm_shape_switch_flag = true;
    bool background_temperatures_confirm_flag = false;
    int background_temperatures_confirm_matrix = 0;
    float m_background_temperatures[MAX_LENGTH] = {};
    int m_current_temperatures[MAX_LENGTH] = {0};
    float m_sbtract_background[MAX_LENGTH] = {0};
    int updateBackgroundTemperature(int *_data, int _idx);
    int calSbtractBackground(float *_sbbg, int *_data, int _idx);
    int m_timestamp = 0;
    int m_timecount = 0;

    PeakInfo peaks_info[MAX_LENGTH];
    PeakInfo result_peak_info[MAX_LENGTH];
    int result_peak_count = 0;
    int findPeaks(float *_arr, int _win, float th);

    ArchInfoArr m_analyse_window[ANALYSE_WINDOW_MAX];
    int m_analyse_window_count = 0;
    int trackArch(ArchInfo &_arch, int _peak_win);
    int detactArch(int *_cur_data, int _idx, float *_subbg, int _MAX_LENGTH, int _peak_win);
    int deleteOldArchData(int _cur_time);

    int8_t m_use_ai_model = 1;
    AiShapeModel *m_shape_model;
    int alarmShapeModel(int _track_id);
    int alarmShape(int _arch_trend_th, float _reliable_arch_ratio_th);
    int alarmTemperatureRise(float _temperature_rise_thre);
};


#endif // end define __ALG_CABLE_TEMPERATURE_DETECTOR_H__
