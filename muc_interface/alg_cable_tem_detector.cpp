#include "alg_cable_tem_detector.h"

#include <string.h>

using namespace std;

CableTemDet::CableTemDet() {

}

CableTemDet::~CableTemDet() {

}

int CableTemDet::init(int _control)
{
    int alarm_switch_flag = _control % 8;
    alarm_gb_switch_flag = alarm_switch_flag & 0b001;
    alarm_temdiff_switch_flag = alarm_switch_flag & 0b010;
    alarm_shape_switch_flag = alarm_switch_flag & 0b100;
    memset(background_temperatures, -999, sizeof(background_temperatures));
    // float *p = background_temperatures;
    // for (int i = 0; i < MAX_LENGTH; i++)
    // {
    //     *p = -999;
    //     p++;
    // }
    return 0;
}

static int alarmGB(int *_data, int _idx)
{
    int res = 0;
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        if (_data[i] > CONSTANT_TEMPERATUE_THRESHOLD)
        {
            res = 1;
            break;
        }
    }
    return res;
}

int CableTemDet::updateBackgroundTemperature(int *_data, int _idx)
{
    int flag = background_temperatures_confirm_matrix & 0b1 << _idx;
    if (flag == 0)
    {
        for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
        {
            background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i];
        }
        background_temperatures_confirm_matrix = background_temperatures_confirm_matrix | 0b1 << _idx;
        return -1; // init background temperatures calculting
    }

    float ALPHA = 0.0001;
    float ALPHA2 = 1 - ALPHA;
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        // TODO
        // judgeInputData();
        background_temperatures[_idx*ONE_GROUP_DATA_LENGTH] = ALPHA * _data[i] +
            ALPHA2 * background_temperatures[_idx*ONE_GROUP_DATA_LENGTH];
    }
    return 0;
}

int CableTemDet::calSbtractBackground(float *_sbbg, int *_data, int _idx)
{
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        if (_data[i] < -40) continue;
        _sbbg[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i] - background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i];
    }
    return 0;
}

int findPeaks(float *_arr, int *_peak_idx, int _win, float th)
{
    int peaks[MAX_LENGTH] = {0};
    // find local max
    if (_arr[0] > _arr[1]) peaks[0] = 1;
    if (_arr[MAX_LENGTH - 1] > _arr[MAX_LENGTH - 2]) peaks[MAX_LENGTH - 1] = 1;
    int step = 0;
    for (int i = 1; i < MAX_LENGTH - 1; i++)
    {
        if (_arr[i] < th) continue;
        if (_arr[i] > _arr[i-1] && _arr[i] > _arr[i+1]) peaks[i] = 1;
        // 如果是平台形状，取中点
        if (_arr[i] >= _arr[i-1] && _arr[i] == _arr[i+1])
        {
            step++;
        }
        if (_arr[i] == _arr[i-1] && _arr[i] > _arr[i+1])
        {
            peaks[i - int(step) / 2] = 1;
            step == 0;
        }
    }
    // selected by win by nms-algorithm
    std::vector<std::pair<int, float>> peaks_info;
    for (int i = 1; i < MAX_LENGTH - 1; i++)
    {
        if (peaks[i])
        {
            peaks_info.push_back(make_pair(i, _arr[i]));
        }
    }
    std::sort(peaks_info.begin(), peaks_info.end(), [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
        return a.second > b.second;
    });

    std::vector<std::pair<int, float>> result_peak_info;
    std::vector<bool> suppressed(peaks_info.size(), false);
    for (size_t i = 0; i < peaks_info.size(); ++i)
    {
        if (suppressed[i]) continue;
        result_peak_info.push_back(peaks_info[i]);
        for (size_t j = i + 1; j < peaks_info.size(); ++j)
        {
            if (abs(peaks_info[i].first - peaks_info[j].first) < 14)
            {
                suppressed[j] = true;
            }
        }
    }
    return 0;
}

static int detactArch()
{
    return 0;
}

static int trackArch()
{
    return 0;
}

static int alarmTemperatureRise()
{
    return 0;
}

static int alarmShape()
{
    return 0;
}

int CableTemDet::run(int *_data, int _idx, int _cable_idx)
{
    //LOGI("entry cable temperature detector algorithm");
    if (_idx * 64 >= MAX_LENGTH || (_cable_idx < 0 || _cable_idx > 3))
    {
        return -1; // index out of max range
    }

    int res = 0;

    if (alarm_gb_switch_flag)
    {
        res = alarmGB(_data, _idx);
    }

    res = updateBackgroundTemperature(_data, _idx);
    if (res < 0)
    {
        return res;
    }

    float sbtract_background[MAX_LENGTH] = {0};
    // TODO calculate sbtract_background
    calSbtractBackground(sbtract_background, _data, _idx);

    int peak_idx[MAX_LENGTH] = {0};
    // TODO
    int FIND_PEAK_WINDOW_SIZE = 16;
    float FIND_PEAK_TEMP_TH = -1.5;//2.5;
    float sbtract_background1[MAX_LENGTH] = {0.0,0.429,0.741,0.741,0.741,0.741,0.741,-0.701,-1.16,-1.43,-1.46,-1.26,-0.879,-0.435,-0.043,0.188,0.189,-0.0515,-0.488,-1.03,-1.54,-1.93,-2.1,-2.03,-1.74,-1.32,-0.88,-0.546,-0.409,-0.515,-0.85,-1.34,-1.89,-2.36,-2.66,-2.73,-2.55,-2.19,-1.75,-1.34,-1.09,-1.05,-1.26,-1.68,-2.21,-2.74,-3.15,-3.35,-3.31,-3.04,-2.63,-2.19,-1.84,-1.67,-1.74,-2.05,-2.53,-3.07,-3.56,-3.89,-3.99,-3.84,-3.5,-3.07,};
    // float sbtract_background1[MAX_LENGTH] = {0.0,0.529,0.941,0.941,0.941,0.941,0.941,-0.000783,-0.357,-0.528,-0.459,-0.156,0.321,0.865,1.36,1.69,1.79,1.65,1.31,0.875,0.456,0.17,0.1,0.275,0.663,1.18,1.72,2.15,2.39,2.38,2.15,1.76,1.31,0.938,0.739,0.774,1.05,1.51,2.05,2.56,2.91,3.05,2.94,2.62,2.19,1.76,1.45,1.35,1.49,1.86,2.37,2.91,3.36,3.63,3.66,3.45,3.07,2.63,2.24,2.01,2.01,2.26,2.7,3.23,};

    res = findPeaks(sbtract_background1, peak_idx, FIND_PEAK_WINDOW_SIZE, FIND_PEAK_TEMP_TH);

    // TODO detect and track arch
    detactArch();
    trackArch();

    // TODO
    // alarmShape
    if (alarm_shape_switch_flag)
    {
        res = alarmShape();
    }

    // TODO
    // alarmTempRise
    if (alarm_temdiff_switch_flag)
    {
        res = alarmTemperatureRise();
    }
    return res;
}