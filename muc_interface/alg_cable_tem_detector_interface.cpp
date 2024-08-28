#include "alg_cable_tem_detector_interface.h"
#include "alg_cable_tem_detector.h"

static CableTemDet cable_tem_detector0;
static CableTemDet cable_tem_detector1;
static CableTemDet cable_tem_detector2;
static CableTemDet cable_tem_detector3;

int alg_cable_temperature_detector_init(int _control)
{
    LOGI("****************************************\r\n");
    LOGI("***** CableTemperarueDetector init *****\r\n");
    LOGI("****************************************\r\n");
    cable_tem_detector0.init(_control);
    LOGI("CableTemperarueDetector [0] init successfully \r\n");
    cable_tem_detector1.init(_control);
    LOGI("CableTemperarueDetector [1] init successfully \r\n");
    cable_tem_detector2.init(_control);
    LOGI("CableTemperarueDetector [2] init successfully \r\n");
    cable_tem_detector3.init(_control);
    LOGI("CableTemperarueDetector [3] init successfully \r\n");
    return 0;
}

static int getAlarmInfo(CableTemDet &det, AlarmInfo *alarm_info)
{
    int count = 0;
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        if (det.m_alarm_status[i] > 0)
        {
            alarm_info[count].alarm_type = det.m_alarm_status[i];
            alarm_info[count].alarm_temp = det.m_alarm_val[i];
            alarm_info[count].addr = i;
            count++;
        }
        if (count >= ALARM_INFO_MAX_NUM) break;
    }
    return count;
}

int alg_cable_temperature_detector_run(int8_t *_data, int _idx, int _cable_idx, uint32_t _timestamp, AlarmInfo *alarm_info)
{
    //LOGD("CableTemDetTest run idx:%d, cable_idx:%d \r\n", _idx, _cable_idx);
    int data[ONE_GROUP_DATA_LENGTH];
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        data[i] = _data[i];
    }
    int idx = _idx / 64;
    int res = 0;

    // 清空报警状态；
    for (int i = 0; i < ALARM_INFO_MAX_NUM; i++)
    {
        alarm_info[i].alarm_type = 0;
        alarm_info[i].alarm_temp = -88;
        alarm_info[i].addr = -99;
    }

    if (_cable_idx == 0)
    {
        res = cable_tem_detector0.run(data, idx, _cable_idx, _timestamp);
        if (res > 0) res = getAlarmInfo(cable_tem_detector0, alarm_info);
    }
    else if (_cable_idx == 1)
    {
        res = cable_tem_detector1.run(data, idx, _cable_idx, _timestamp);
        if (res > 0) res = getAlarmInfo(cable_tem_detector1, alarm_info);
    }
    else if (_cable_idx == 2)
    {
        res = cable_tem_detector2.run(data, idx, _cable_idx, _timestamp);
        if (res > 0) res = getAlarmInfo(cable_tem_detector2, alarm_info);
    }
    else if (_cable_idx == 3)
    {
        res = cable_tem_detector3.run(data, idx, _cable_idx, _timestamp);
        if (res > 0) res = getAlarmInfo(cable_tem_detector3, alarm_info);
    }
    else
    {
        res = -10;
    }

    return res;
}
