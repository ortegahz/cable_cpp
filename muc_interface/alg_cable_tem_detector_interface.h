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

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>


int alg_cable_temperature_detector_init(int _control);

int alg_cable_temperature_detector(float *_data, int _idx, int _cable_idx);

#ifdef __cplusplus
}
#endif

#endif // end define __ALG_CABLE_TEMPERATURE_DETECTOR_H__