#include "alg_cable_tem_detector.h"

#include <string.h>
#include <cmath>

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
    // memset(m_background_temperatures, -999, sizeof(m_background_temperatures));
    // memset(m_current_temperatures, -999, sizeof(m_current_temperatures));
    return 0;
}

static int alarmGB(int *_data, int _idx)
{
    int res = 0;
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        if (_data[i] > ALARM_CONSTANT_TEMPERATUE_THRESHOLD)
        {
            LOGI("alarm gb: %d \r\n", _data[i]);
            res = 1;
            break;
        }
    }
    return res;
}

int CableTemDet::updateBackgroundTemperature(int *_data, int _idx)
{
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        m_current_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i];
    }

    if (m_timecount > TIME_INIT_CNT_TH)
    {
        background_temperatures_confirm_matrix = pow(2, MAX_LENGTH / 64) - 1;
    }
    int flag = background_temperatures_confirm_matrix & 0b1 << _idx;
    if (flag == 0)
    {
        for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
        {
            m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i];
        }
        background_temperatures_confirm_matrix = background_temperatures_confirm_matrix | 0b1 << _idx;
        return -1; // init background temperatures calculting
    }
    if (!background_temperatures_confirm_flag)
    {
        if (background_temperatures_confirm_matrix == pow(2, MAX_LENGTH / 64) - 1)
        {
            background_temperatures_confirm_flag = true;
            LOGI("======================== background_temperatures init full ========================\n");
        }
        else
        {
            return -1;
        }
    }

    float ALPHA = 0.0001;
    float ALPHA2 = 1 - ALPHA;
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i] = ALPHA * _data[i] +
            ALPHA2 * m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i];
    }


    return 0;
}

int CableTemDet::calSbtractBackground(float *_sbbg, int *_data, int _idx)
{
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        if (_data[i] < -40) continue;
        _sbbg[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i] - m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i];
    }
    return 0;
}

std::vector<int> findPeaks(float *_arr, int _win, float th)
{
    int peaks[MAX_LENGTH] = {0};
    // step1. find local max
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
            step = 0;
        }
    }
    // step2. selected by win by nms-algorithm
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
            if (abs(peaks_info[i].first - peaks_info[j].first) < _win)
            {
                suppressed[j] = true;
            }
        }
    }
    std::vector<int> peak_idx;
    for (size_t i = 0; i < result_peak_info.size(); ++i)
    {
        peak_idx.push_back(result_peak_info[i].first);
    }
    return peak_idx;
}

static int calculateArchTrend(float *_arr, int _peak, int _p_start, int _p_end, float th=0.5)
{
    int trend_score = 0;
    float diff;

    for (int i = _p_start; i < _peak; i++)
    {
        if (_arr[i] < -th) trend_score--;
        else
        {
            diff = _arr[i + 1] - _arr[i];
            if (abs(diff) >= th && _arr[i + 1] >= -th)
            {
                if (diff > 0) trend_score++;
                else if(diff < 0) trend_score--;
            }
        }
    }
    for (int i = _peak; i < _p_end; i++)
    {
        if (_arr[i] < -th) trend_score--;
        else
        {
            diff = _arr[i] - _arr[i + 1];
            if (abs(diff) >= th && _arr[i + 1] >= -th)
            {
                if (diff > 0) trend_score++;
                else if(diff < 0) trend_score--;
            }
        }
    }
    return trend_score;
}

int CableTemDet::trackArch(ArchInfo &_arch, int _peak_win=16)
{
    int track_id = -1;
    int mid_distance = _peak_win / 2;
    for (size_t i = 0; i < m_analyse_window.size(); i++)
    {
        int last_id = m_analyse_window[i].archs.size() -1;
        if (abs(m_analyse_window[i].archs[last_id].peak - _arch.peak) < mid_distance)
        {
            track_id = m_analyse_window[i].arch_id;
        }
    }
    if (track_id == -1)
    {
        track_id = m_analyse_window.size();
    }
    return track_id;
}

int CableTemDet::detactArch(int *_cur_data, int _idx, float *_subbg, std::vector<int> _peak_idx, int _MAX_LENGTH, int _peak_win=16)
{
    for (size_t i = 0; i < _peak_idx.size(); i++)
    {
        int peak = _peak_idx[i];
        int p_start = max(0, peak - _peak_win/2);
        int p_end = min(_MAX_LENGTH - 1, peak + _peak_win/2);
        int arch_trend = calculateArchTrend(_subbg, peak, p_start, p_end);
        ArchInfo cur_arch;
        cur_arch.peak = peak;
        cur_arch.peak_val = m_current_temperatures[peak];
        cur_arch.p_start = p_start;
        cur_arch.p_end = p_end;
        cur_arch.arch_trend = arch_trend;
        cur_arch.timestamp = m_timestamp;
        int win_start = max(0, peak - SAVE_ACTULLY_BUFFER_SIZE/2);
        int win_end = min(_MAX_LENGTH, peak + SAVE_ACTULLY_BUFFER_SIZE/2);
        for (int j = win_start; j < win_end; j++)
        {
            cur_arch.temp[j - win_start] = m_current_temperatures[j];
        }
        int track_id = trackArch(cur_arch, _peak_win);
        if (track_id >= m_analyse_window.size())
        {
            ArchInfoArr new_archs;
            new_archs.arch_id = track_id;
            new_archs.archs.push_back(cur_arch);
            m_analyse_window.push_back(new_archs);
        }
        else if (track_id >= 0)
        {
            m_analyse_window[track_id].archs.push_back(cur_arch);
        }
        if (m_analyse_window[track_id].archs.size() < 2) continue;
        // LOGD("track_id: %d, 累计时长：%d", m_analyse_window[track_id].arch_id, m_analyse_window[track_id].archs[m_analyse_window[track_id].archs.size()-1].timestamp - m_analyse_window[track_id].archs[0].timestamp);
    }
    return 0;
}

int CableTemDet::deleteOldArchData(int _cur_time)
{
    for (size_t i = 0; i < m_analyse_window.size(); i++)
    {
        int delete_p_end = -1;
        if (m_analyse_window[i].archs.size() >= SAVE_ARCHS_MAX_NUM_PER_PEAK)
        {
            delete_p_end = 30;
        }
        if (m_timestamp - m_analyse_window[i].archs[m_analyse_window[i].archs.size() - 1].timestamp> BEAR_TIME_THRESHOLD)
        {
            delete_p_end = m_analyse_window[i].archs.size() - 1;
        }
        if (delete_p_end >= 0)
        {
            m_analyse_window[i].archs.erase(m_analyse_window[i].archs.begin(), m_analyse_window[i].archs.begin() + delete_p_end);
        }
    }
    return 0;
}

int CableTemDet::alarmShape(int _arch_trend_th=4, float _reliable_arch_ratio_th=0.8)
{
    int res = 0;
    for (size_t i = 0; i < m_analyse_window.size(); i++)
    {
        if (m_analyse_window[i].archs.size() < ACC_TIME_THRESHOLD) continue;
        int reliable_arch_count = 0;
        int end_id = max(0, int(m_analyse_window[i].archs.size() - ACC_TIME_THRESHOLD));
        int used_arch_num = m_analyse_window[i].archs.size() - end_id;
        for (int j = (m_analyse_window[i].archs.size() - 1); j >= end_id ; j--)
        {
            reliable_arch_count += m_analyse_window[i].archs[j].arch_trend > _arch_trend_th;
        }
        int alarm = 0;
        if (float(reliable_arch_count) / used_arch_num > _reliable_arch_ratio_th)
        {
            alarm = 1;
            res = 10;
        }
        LOGD("[%d] alarm shape: track_id: %d, alarm: %d, reliable_arch_count：%d, ratio: %.2f, peakid: %d \r\n", m_idx, m_analyse_window[i].arch_id, alarm, reliable_arch_count, float(reliable_arch_count) / used_arch_num, m_analyse_window[i].archs[m_analyse_window[i].archs.size() - 1].peak);
    }
    return res;
}

int CableTemDet::alarmTemperatureRise(float _temperature_rise_thre=1)
{
    int alarm = 0;
    for (size_t i = 0; i < m_analyse_window.size(); i++)
    {
        if (m_analyse_window[i].archs.size() < 2) continue;
        int last_idx = m_analyse_window[i].archs.size() - 1;
        int start_id = 9999;
        for (int j = m_analyse_window[i].archs.size() - 2; j >= 0; j--)
        {
            if (m_analyse_window[i].archs[last_idx].timestamp - m_analyse_window[i].archs[j].timestamp > ACC_TIME_THRESHOLD)
            {
                break;
            }
            start_id = j;
        }
        if (start_id == 9999) continue;

        float start_arch_val_max = -999.0;
        float last_arch_val_max = -999.0;
        for (int j = 0; j < SAVE_ACTULLY_BUFFER_SIZE; j++)
        {
            if (m_analyse_window[i].archs[start_id].temp[j] > start_arch_val_max) start_arch_val_max = m_analyse_window[i].archs[start_id].temp[j];
            if (m_analyse_window[i].archs[last_idx].temp[j] > last_arch_val_max) last_arch_val_max = m_analyse_window[i].archs[last_idx].temp[j];
        }
        float temp_diff = last_arch_val_max - start_arch_val_max;
        float time_diff = m_analyse_window[i].archs[last_idx].timestamp - m_analyse_window[i].archs[start_id].timestamp;
        if (time_diff < ACC_TIME_THRESHOLD) continue;
        if ((temp_diff / time_diff) * 60 > _temperature_rise_thre)
        {
            alarm = 1;
        }
        LOGD("[%d] alarm temp rise: track_id: %d, archs num: %d, alarm: %d, temp_diff：%.1f, time_diff: %.1f, rise_rate: %.2f, val1:%.2f, val2:%.2f, peakid: %d \n", m_idx, m_analyse_window[i].arch_id, m_analyse_window[i].archs.size(), alarm, temp_diff, time_diff, (temp_diff / time_diff) * 60, last_arch_val_max, start_arch_val_max, m_analyse_window[i].archs[m_analyse_window[i].archs.size() - 1].peak);
    }
    return alarm;
}

int CableTemDet::run(int *_data, int _idx, int _cable_idx, uint32_t _timestamp)
{
    // LOGD("entry cable temperature detector algorithm idx:%d, cable_idx:%d \r\n", _idx, _cable_idx);

    m_idx = _idx;
    m_timestamp = _timestamp;
    m_timecount++;
    int res = 0;
    int alarm = 0;
    if (_idx * 64 >= MAX_LENGTH || (_cable_idx < 0 || _cable_idx > 3))
    {
        return -1; // index out of max range
    }

    if (alarm_gb_switch_flag)
    {
        res = alarmGB(_data, _idx);
        if (res > 0) alarm = 1;
    }

    res = updateBackgroundTemperature(_data, _idx);
    if (res < 0)
    {
        return res;
    }

    // 等待大概4分钟，等本底值更新几次后，再开始报警逻辑
    if (m_timecount <= TIME_RUN_CNT_TH)
    {
        return -1;
    }

    // 计算当前值与本底值的温度差，由于一次只传入64个点的数据，其他点差值为0
    float sbtract_background[MAX_LENGTH] = {0};
    calSbtractBackground(sbtract_background, _data, _idx);

    // 寻找差值数据的波峰
    std::vector<int> peak_idx;
    peak_idx = findPeaks(sbtract_background, FIND_PEAK_WINDOW_SIZE, FIND_PEAK_TEMP_TH);

    // 检测与跟踪算法的关键参数----拱形(arch)
    detactArch(_data, _idx, sbtract_background, peak_idx, MAX_LENGTH);

    //删除好久没用的arch数据，减少内存占用
    deleteOldArchData(1);

    // 形状报警
    if (alarm_shape_switch_flag)
    {
        res = alarmShape(ALARM_ARCH_TREND_TH, ALARM_ARCH_RATIO_TH);
        if (res > 0) alarm += 10;
    }

    // 升温报警
    if (alarm_temdiff_switch_flag)
    {
        res = alarmTemperatureRise(ALARM_TEMPERATURE_RISE_THRESHLOD);
        if (res > 0) alarm += 100;
    }

    if (alarm > 0) return alarm;
    else return res;
}