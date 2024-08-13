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

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

/**
 * @description:
 * @param int _control
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
 * @return {0: normal, 1: alarm, <0: error}
 */
int alg_cable_temperature_detector_run(int8_t *_data, int _idx, int _cable_idx, uint32_t _timestamp);

#ifdef __cplusplus
}
#endif

#endif // end define __ALG_CABLE_TEMP_DETECTOR_INTERFACE_H__