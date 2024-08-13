#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include "alg_cable_tem_detector_interface.h"
#include "alg_cable_tem_detector.h"


static CableTemDet cable_tem_detector0;
static CableTemDet cable_tem_detector1;
static CableTemDet cable_tem_detector2;
static CableTemDet cable_tem_detector3;

int alg_cable_temperature_detector_init(int _control)
{
    LOGI("****************************************\n");
    LOGI("***** CableTemperarueDetector init *****\n");
    LOGI("****************************************\n");
    cable_tem_detector0.init(_control);
    LOGI("CableTemperarueDetector [0] init successfully \n");
    cable_tem_detector1.init(_control);
    LOGI("CableTemperarueDetector [1] init successfully \n");
    cable_tem_detector2.init(_control);
    LOGI("CableTemperarueDetector [2] init successfully \n");
    cable_tem_detector3.init(_control);
    LOGI("CableTemperarueDetector [3] init successfully \n");
    return 0;
}

int alg_cable_temperature_detector_run(int8_t *_data, int _idx, int _cable_idx, uint32_t _timestamp)
{
    LOGI("CableTemDetTest run idx:%d, cable_idx:%d \r\n", _idx, _cable_idx);
    int data[ONE_GROUP_DATA_LENGTH];
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        data[i] = _data[i];
    }

    int res = 0;
    if (_cable_idx == 0)
    {
        res = cable_tem_detector0.run(data, _idx, _cable_idx, _timestamp);
    }
    else if (_cable_idx == 1)
    {
        res = cable_tem_detector1.run(data, _idx, _cable_idx, _timestamp);
    }
    else if (_cable_idx == 2)
    {
        res = cable_tem_detector2.run(data, _idx, _cable_idx, _timestamp);
    }
    else if (_cable_idx == 3)
    {
        res = cable_tem_detector3.run(data, _idx, _cable_idx, _timestamp);
    }
    else
    {
        res = -10;
    }

    return res;
}
