/*
 * @Author: wx
 * @Date: 2024-08-05 15:28:25
 * @LastEditors: wx
 * @LastEditTime: 2024-08-05 15:28:25
 * @FilePath: alg_cable_tem_detector.h
 * @Description:
 */

#ifndef __ALG_CABLE_TEMP_DETECTOR_INTERFACE_H__
#define __ALG_CABLE_TEMP_DETECTOR_INTERFACE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define ALARM_INFO_MAX_NUM 16
typedef struct {
    uint8_t alarm_type;
    int8_t alarm_temp;
    uint16_t addr;
} AlarmInfo;


/**
 * @description:
 * @param int _control: 控制是否报警
 *    0b000：不报警
 *    0b001：国标报警
 *    0b010：升温报警
 *    0b100：形状报警
 *    0b111：全部标准都报警
 * @return {0: normal}
 */
int alg_cable_temperature_detector_init(int _control);

/**
 * @description:
 * @param
 *    int *_data: 温度数据，必须64长度
 *    int _idx:   位置索引
 *    int _cable_idx: 那一条线缆
 *    uint32_t timestamp:  时间戳
 *    AlarmInfo *alarm_info 报警信息
 *    int8_t _use_ai_model: 是否启用AI卷积模型做形状判断，1使能；0失能
 * @return {0: normal, <0: error/没初始化完，>0: 报警点的数量, 依据数量从}
 */
int alg_cable_temperature_detector_run(int8_t *_data, int _idx, int _cable_idx, uint32_t _timestamp, AlarmInfo *alarm_info, int8_t _use_ai_model);

#ifdef __cplusplus
}
#endif

#endif // end define __ALG_CABLE_TEMP_DETECTOR_INTERFACE_H__
