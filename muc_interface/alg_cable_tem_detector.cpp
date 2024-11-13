#include <string.h>
#include <cmath>
#include <unordered_set>
#include "alg_cable_tem_detector.h"


using namespace std;

// 全局的ShapeModel变量，因为有4条线缆要处理，但是AI模型比较占资源，所以只定义一个供4条线缆使用。
static AiShapeModel g_shape_model;
static float g_shape_model_input_data[SHAPE_MODEL_INPUT_SIZE] = {0.0f};

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
    m_init_flag = true;

    // init shape ai model
    m_shape_model = &g_shape_model;
    m_shape_model->init();
    // g_shape_model.init();

    for (int i = 0; i < MAX_LENGTH; i++) m_background_temperatures[i] = -100;

    return 0;
}

static int alarmGB(int *_data, int8_t *_alarm_status, int8_t *_alarm_val, int _idx)
{
    int res = 0;
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        if (_data[i] > ALARM_CONSTANT_TEMPERATUE_THRESHOLD && _data[i] < 120)
        {
            LOGD("alarm gb: %d \r\n", _data[i]);
            _alarm_status[_idx*ONE_GROUP_DATA_LENGTH + i] = 2;
            _alarm_val[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i];
            res = 1;
        }
    }
    return res;
}

static int judgeAnormalPoint(int _val)
{
    int res = 0;
    if (_val < -90)
    {
        res = -1;
    }
    return res;
}

void processAbnormalData(int *_data)
{
    for (int i = 1; i < ONE_GROUP_DATA_LENGTH - 1; i++)
    {
        if (_data[i] == 43 && (judgeAnormalPoint(_data[i-1]) < 0 || judgeAnormalPoint(_data[i+1]) < 0))
        {
            _data[i] = -95;
        }
    }
}

static void unanormalPointSuppression(int *_data, int8_t *_alarm_status, int8_t *_alarm_val, int _idx, bool _alarm_suppression)
{
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        if (judgeAnormalPoint(_data[i]) < 0 && _alarm_status[i] != 0)
        {
            _alarm_status[i] = 0;
            // LOGD("unanormalPointSuppression val:%d, peak:%d\r\n", _data[i], i);
        }
        if (_alarm_suppression) _alarm_status[i] = 0;
    }
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
            if (judgeAnormalPoint(_data[i]) >= 0)
            {
                m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i];
            }
        }
        background_temperatures_confirm_matrix = background_temperatures_confirm_matrix | 0b1 << _idx;
        return -1; // init background temperatures calculting
    }
    if (!background_temperatures_confirm_flag)
    {
        if (background_temperatures_confirm_matrix == pow(2, MAX_LENGTH / 64) - 1)
        {
            background_temperatures_confirm_flag = true;
            LOGI("======================== background_temperatures init full ========================\r\n");
        }
        else
        {
            m_timestamp_last = m_timestamp;
            return -11;
        }
    }

    int _update_cnt = m_timestamp - m_timestamp_last + 1;
    for (int z = 0; z < _update_cnt; z++)
    {
        float ALPHA = 0.005; // 本底更新约30min完成完整更新
        float ALPHA2 = 1 - ALPHA;
        for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
        {
            if (judgeAnormalPoint(_data[i]) >= 0)
            {
                if (m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i] < -90)
                {
                    m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i] = _data[i];
                }
                else
                {
                    m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i] = ALPHA * _data[i] +
                                                                                  ALPHA2 * m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i];
                }
            }
        }
    }
    m_timestamp_last = m_timestamp;

//    float ALPHA = 0.005; // 本底更新约30min完成完整更新
//    float ALPHA2 = 1 - ALPHA;
//    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
//    {
//        if (judgeAnormalPoint(_data[i]) >= 0)
//        {
//            if (m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i] < -90)
//            {
//                m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i] = _data[i];
//            }
//            else
//            {
//                m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i] = ALPHA * _data[i] +
//                                                                              ALPHA2 * m_background_temperatures[_idx * ONE_GROUP_DATA_LENGTH + i];
//            }
//        }
//    }

    return 0;
}

int CableTemDet::calSbtractBackground(float *_sbbg, int *_data, int _idx)
{
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        if (judgeAnormalPoint(_data[i])) {
            this->m_points_status[_idx*ONE_GROUP_DATA_LENGTH + i] = 180;
            m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i] = 0;
            _sbbg[_idx*ONE_GROUP_DATA_LENGTH + i] = 0;
        } else {
            if (this->m_points_status[_idx*ONE_GROUP_DATA_LENGTH + i] == 0) {
                _sbbg[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i] - m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i];
            } else {
                this->m_points_status[_idx*ONE_GROUP_DATA_LENGTH + i]--;
                m_background_temperatures[_idx*ONE_GROUP_DATA_LENGTH + i] = _data[i];
                _sbbg[_idx*ONE_GROUP_DATA_LENGTH + i] = 0;
            }
        }
    }
    int cnt = 0;
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        if (_sbbg[_idx*ONE_GROUP_DATA_LENGTH + i] > FIND_PEAK_TEMP_TH)
        {
            cnt += 1;
        }
    }
    // LOGD("_sbbg cnt: %d \r\n", cnt);

    // 整体升温时报警抑制
    if (cnt > ONE_GROUP_DATA_LENGTH / 3)
    {
        m_alarm_suppression = true;
    }
    // for (int i = 0; i < MAX_LENGTH; i++)
    // {
    //     if (judgeAnormalPoint(m_current_temperatures[i])) continue;
    //     _sbbg[i] = m_current_temperatures[i] - m_background_temperatures[i];
    // }
    return 0;
}

static int compare(const void* a, const void* b)
{
    PeakInfo* pa = (PeakInfo*)a;
    PeakInfo* pb = (PeakInfo*)b;
    return (pa->value < pb->value) - (pa->value > pb->value);  // 降序排序
}

int CableTemDet::findPeaks(float *_arr, int _win, float th)
{
    static int peaks[MAX_LENGTH] = {0};
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        peaks[i] = 0;
    }

    // step1. find local max
    if (_arr[0] > _arr[1] && _arr[0] >= th && judgeAnormalPoint(m_current_temperatures[1]) >= 0) peaks[0] = 1;
    if (_arr[MAX_LENGTH - 1] > _arr[MAX_LENGTH - 2] && _arr[MAX_LENGTH - 1] >= th && judgeAnormalPoint(m_current_temperatures[MAX_LENGTH - 2]) >= 0) peaks[MAX_LENGTH - 1] = 1;
    int step = 0;
    // float th1 = 0.5;
    for (int i = 1; i < MAX_LENGTH - 1; i++)
    {
        if (_arr[i] < th) continue;
        if (judgeAnormalPoint(m_current_temperatures[i-1]) < 0) continue;
        if (judgeAnormalPoint(m_current_temperatures[i+1]) < 0) continue;
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
    int peaks_count = 0;
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        if (peaks[i])
        {
            peaks_info[peaks_count].index = i;
            peaks_info[peaks_count].value = _arr[i];
            peaks_count++;
        }
    }

    qsort(peaks_info, peaks_count, sizeof(PeakInfo), compare);

    static bool suppressed[MAX_LENGTH];
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        suppressed[i] = false;
    }
    result_peak_count = 0;

    for (int i = 0; i < peaks_count; i++)
    {
        if (suppressed[i]) continue;
        result_peak_info[result_peak_count] = peaks_info[i];
        result_peak_count++;
        for (int j = i + 1; j < peaks_count; j++)
        {
            if (abs(peaks_info[i].index - peaks_info[j].index) < _win)
            {
                suppressed[j] = true;
            }
        }
    }

    return result_peak_count;
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
    for (size_t i = 0; i < m_analyse_window_count; i++)
    {
        int last_id = m_analyse_window[i].arch_count -1;
        if (abs(m_analyse_window[i].archs[last_id].peak - _arch.peak) < mid_distance)
        {
            track_id = m_analyse_window[i].arch_id;
        }
    }
    if (track_id == -1)
    {
        track_id = m_analyse_window_count;
    }
    return track_id;
}

int CableTemDet::detactArch(int *_cur_data, int _idx, float *_subbg, int _MAX_LENGTH, int _peak_win)
{
    for (size_t i = 0; i < result_peak_count; i++)
    {
        int peak = result_peak_info[i].index;
        int p_start = fmax(0, peak - _peak_win / 2);
        int p_end = fmin(MAX_LENGTH - 1, peak + _peak_win / 2);
        int arch_trend = calculateArchTrend(_subbg, peak, p_start, p_end);

        ArchInfo cur_arch;
        cur_arch.peak = peak;
        cur_arch.peak_val = m_current_temperatures[peak];
        cur_arch.p_start = p_start;
        cur_arch.p_end = p_end;
        cur_arch.arch_trend = arch_trend;
        cur_arch.timestamp = m_timestamp;

        int win_start = fmax(0, peak - SAVE_ACTULLY_BUFFER_SIZE / 2);
        int win_end = fmin(MAX_LENGTH, peak + SAVE_ACTULLY_BUFFER_SIZE / 2);
        for (int j = win_start; j < win_end; j++)
        {
            cur_arch.temp[j - win_start] = m_current_temperatures[j];
            cur_arch.temp_sub[j - win_start] = _subbg[j];
        }

        int track_id = trackArch(cur_arch, _peak_win);

        if (track_id >= m_analyse_window_count && m_analyse_window_count < ANALYSE_WINDOW_MAX)
        {
            m_analyse_window[track_id].arch_id = track_id;
            m_analyse_window[track_id].arch_count = 0;
            m_analyse_window[track_id].archs[m_analyse_window[track_id].arch_count++] = cur_arch;
            m_analyse_window_count++;
        }
        else if (track_id >= 0 && track_id < ANALYSE_WINDOW_MAX)
        {
            m_analyse_window[track_id].archs[m_analyse_window[track_id].arch_count++] = cur_arch;
        }
        else
        {
            continue;
        }

        if (m_analyse_window[track_id].arch_count < 2) continue;
        // LOGD("track_id: %d, 累计时长：%d", m_analyse_window[track_id].arch_id, m_analyse_window[track_id].archs[m_analyse_window[track_id].arch_count-1].timestamp - m_analyse_window[track_id].archs[0].timestamp);
    }
    return 0;
}

int CableTemDet::deleteOldArchData(int _cur_time)
{
    for (size_t i = 0; i < m_analyse_window_count; i++)
    {
        int delete_p_end = -1;
        if (m_analyse_window[i].arch_count >= SAVE_ARCHS_MAX_NUM_PER_PEAK)
        {
            delete_p_end = SAVE_ARCHS_MAX_NUM_PER_PEAK - ACC_TIME_THRESHOLD;
        }
        if (m_timestamp - m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].timestamp > BEAR_TIME_THRESHOLD)
        {
            delete_p_end = m_analyse_window[i].arch_count - 1;
        }
        if (delete_p_end > 0)
        {
            int rest_count = m_analyse_window[i].arch_count - delete_p_end;
            for (int j = 0; j < rest_count; j++)
            {
                m_analyse_window[i].archs[j] = m_analyse_window[i].archs[j+1];
            }
            m_analyse_window[i].arch_count -= delete_p_end;
        }
    }
    return 0;
}

int CableTemDet::alarmShapeModel(int _track_id)
{
    int res = 0;
    int last_idx = m_analyse_window[_track_id].arch_count - 1;
    for (int i = 0; i < SHAPE_MODEL_INPUT_HEIGHT; i++)
    {
        for (int j = 0; j < SHAPE_MODEL_INPUT_WIDTH; j++)
        {
            g_shape_model_input_data[i*SHAPE_MODEL_INPUT_WIDTH + j] = m_analyse_window[_track_id].archs[last_idx - i].temp_sub[j];
            //printf("%.2f,", g_shape_model_input_data[i*SHAPE_MODEL_INPUT_WIDTH + j]);
        }
        //printf("\r\n");
    }

    float output_data[2];
    res = m_shape_model->run(g_shape_model_input_data, output_data);
    for(int i = 0; i < SHAPE_MODEL_PROBABILITY_BUFFER_SIZE - 1; i++) m_analyse_window[_track_id].shape_prob_buffer[i] = m_analyse_window[_track_id].shape_prob_buffer[i+1];
    if (res == 1)
    {
        m_analyse_window[_track_id].shape_prob_buffer[SHAPE_MODEL_PROBABILITY_BUFFER_SIZE - 1] = output_data[1];
    }
    else
    {
        m_analyse_window[_track_id].shape_prob_buffer[SHAPE_MODEL_PROBABILITY_BUFFER_SIZE - 1] = 0;
    }
    float avg_prob = 0;
    for(int i = 0; i < SHAPE_MODEL_PROBABILITY_BUFFER_SIZE; i++) avg_prob += m_analyse_window[_track_id].shape_prob_buffer[i];
    avg_prob = avg_prob / SHAPE_MODEL_PROBABILITY_BUFFER_SIZE;
    int alarm = 0;
    if (avg_prob > SHAPE_MODEL_AVERAGE_PROB_TH)
    {
        alarm = 1;
    }
    LOGD("[DEBUG] MODEL pred:%d, [%d]: %.2f, [%d]: %.2f, alarm:%d, avg:%.2f \r\n", res, 0, output_data[0], 1, output_data[1], alarm, avg_prob);
    return alarm;
}

int CableTemDet::alarmShape(int _arch_trend_th=4, float _reliable_arch_ratio_th=0.8)
{
    int res = 0;
    for (size_t i = 0; i < m_analyse_window_count; i++)
    {
        for (int j = 0; j <m_analyse_window[i].arch_count; j++)
        {
            //LOGD("[DEBUG] m_analyse_window[%d], peak:%d, %d\r\n", i, m_analyse_window[i].archs[j].peak, (&m_analyse_window[i].archs[j].peak));
        }

        if (m_analyse_window[i].arch_count < ACC_TIME_THRESHOLD) continue;
        int reliable_arch_count = 0;
        int end_id = max(0, int(m_analyse_window[i].arch_count - ACC_TIME_THRESHOLD));
        int used_arch_num = m_analyse_window[i].arch_count - end_id;
        for (int j = (m_analyse_window[i].arch_count - 1); j >= end_id ; j--)
        {
            reliable_arch_count += m_analyse_window[i].archs[j].arch_trend > _arch_trend_th;
        }
        int alarm = 0;
        if (float(reliable_arch_count) / used_arch_num > _reliable_arch_ratio_th)
        {
            int alarm_model = 1;
            if (m_use_ai_model)
            {
                alarm_model = alarmShapeModel(i);
            }
            if (alarm_model)
            {
                alarm = 1;
                res = 10;
                m_alarm_status[m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak] = 3;
                m_alarm_val[m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak] = m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak_val;
            }
            LOGD("[DEBUG][%d] shape track_id: %d alarm: %d, cnt:%d\r\n", m_idx, m_analyse_window[i].arch_id, alarm, reliable_arch_count);
        }
        //LOGD("[DEBUG][%d] alarm shape: track_id: %d, alarm: %d, reliable_arch_count：%d, ratio: %.2f, peakid: %d \r\n", m_idx, m_analyse_window[i].arch_id, alarm, reliable_arch_count, float(reliable_arch_count) / used_arch_num, m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak);
    }
    return res;
}

int CableTemDet::alarmTemperatureRise(float _temperature_rise_thre)
{
    int alarm = 0;
    for (size_t i = 0; i < m_analyse_window_count; i++)
    {
        if (m_analyse_window[i].arch_count < 2) continue;
        int last_idx = m_analyse_window[i].arch_count - 1;
        int start_id = 9999;
        for (int j = m_analyse_window[i].arch_count - 2; j >= 0; j--)
        {
            if (m_analyse_window[i].archs[last_idx].timestamp - m_analyse_window[i].archs[j].timestamp >= ACC_TIME_THRESHOLD && (last_idx - start_id >= ACC_TIME_THRESHOLD))
            {
                break;
            }
            start_id = j;
        }
        if (start_id == 9999) continue;
        if (last_idx - start_id < ACC_TIME_THRESHOLD / 2) continue;

        float start_arch_val_max = -999.0;
        float last_arch_val_max = -999.0;
        for (int j = 0; j < SAVE_ACTULLY_BUFFER_SIZE; j++)
        {
            if (m_analyse_window[i].archs[start_id].temp[j] > start_arch_val_max) start_arch_val_max = m_analyse_window[i].archs[start_id].temp[j];
            if (m_analyse_window[i].archs[last_idx].temp[j] > last_arch_val_max) last_arch_val_max = m_analyse_window[i].archs[last_idx].temp[j];
        }

        std::unordered_set<float> arch_val_max_set;
        for (int k = start_id; k <= last_idx; k++) {
            float arch_val_max = -999.0;
            for (int j = 0; j < SAVE_ACTULLY_BUFFER_SIZE; j++) {
                if (m_analyse_window[i].archs[k].temp[j] > arch_val_max) arch_val_max = m_analyse_window[i].archs[k].temp[j];
            }
            arch_val_max_set.insert(arch_val_max);
        }

        float temp_diff = last_arch_val_max - start_arch_val_max;
        float time_diff = m_analyse_window[i].archs[last_idx].timestamp - m_analyse_window[i].archs[start_id].timestamp;
//        std::cout << "timestamp last " << m_analyse_window[i].archs[last_idx].timestamp << " & timestamp start " << m_analyse_window[i].archs[start_id].timestamp << std::endl;
        if (time_diff < ACC_TIME_THRESHOLD) continue;
        if ((temp_diff / time_diff) * 60 > _temperature_rise_thre && arch_val_max_set.size() >= 3)
        {
            alarm = 1;
            m_alarm_status[m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak] = 1;
            m_alarm_val[m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak] = m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak_val;
            // notice: LOG不要太复杂，否则可能栈爆掉
            LOGD("[DEBUG][%d] rise: track_id: %d, archs num: %d, alarm: %d, rise_rate: %.2f, peakid: %d\r\n",m_idx, m_analyse_window[i].arch_id, m_analyse_window[i].arch_count, alarm,  (temp_diff / time_diff) * 60, m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak);
        }
//        LOGD("[DEBUG][%d] alarm temp rise: track_id: %d, archs num: %d, alarm: %d, temp_diff：%.1f, time_diff: %.1f, rise_rate: %.2f, val1:%.2f, val2:%.2f, peakid: %d \r\n", m_idx, m_analyse_window[i].arch_id, m_analyse_window[i].arch_count, alarm, temp_diff, time_diff, (temp_diff / time_diff) * 60, last_arch_val_max, start_arch_val_max, m_analyse_window[i].archs[m_analyse_window[i].arch_count - 1].peak);
    }
    return alarm;
}

int CableTemDet::run_all(int *_data, int _idx, int _cable_idx, uint32_t _timestamp, int8_t _use_ai_model)
{
    int res = 0;

    // update m_current_temperatures
    for (int i = 0; i < ONE_GROUP_DATA_LENGTH; i++)
    {
        m_current_temperatures[_idx + i] = _data[i];
    }

    if (m_timestamp_suppose < 0) m_timestamp_suppose = _timestamp;

    if (_timestamp < m_timestamp_suppose) return res;

    while (_timestamp >= m_timestamp_suppose)
    {
        for (int i = 0; i < MAX_LENGTH; i += ONE_GROUP_DATA_LENGTH) {
            int data[ONE_GROUP_DATA_LENGTH];
            int data_m[ONE_GROUP_DATA_LENGTH];
            for (int j = 0; j < ONE_GROUP_DATA_LENGTH; j++) {
//                data[j] = _data[j];
                data[j] = m_current_temperatures[i + j];
            }
            int idx = i / ONE_GROUP_DATA_LENGTH;
//            if (idx != 8) continue;

//            std::cout << "data_k -->";
//            for (int k = 0; k < ONE_GROUP_DATA_LENGTH; k++) {
//                std::cout << " " << data[k];
//            }
//            std::cout << std::endl;
//
//            std::cout << "data_m -->";
//            for (int k = 0; k < ONE_GROUP_DATA_LENGTH; k++) {
//                std::cout << " " << data_m[k];
//            }
//            std::cout << std::endl;

//            std::cout << "[cmp] idx -->" << idx << std::endl;
//            std::cout << "[cmp] m_timestamp_suppose -->" << m_timestamp_suppose << std::endl;
            if (this->run(data, idx, _cable_idx, m_timestamp_suppose, _use_ai_model) > 0) res++;
//            printf("m_alarm_status --> %d \r\n", m_alarm_status[570]);
        }
        if (res > 0) return res;
//        std::cout << "m_timestamp_suppose -->" << m_timestamp_suppose << std::endl;
        m_timestamp_suppose++;  // add 1s
    }

    return res;
}

                    // 数据      组号         线缆号
int CableTemDet::run(int *_data, int _idx, int _cable_idx, uint32_t _timestamp, int8_t _use_ai_model)
{
    // LOGD("entry cable temperature detector algorithm idx:%d, cable_idx:%d \r\n", _idx, _cable_idx);
    if (!m_init_flag)  //初始化
    {
        return -1;
    }

//    std::cout << "_data -->";
//    for (int k = 0; k < ONE_GROUP_DATA_LENGTH; k++) {
//        std::cout << " " << _data[k];
//    }
//    std::cout << std::endl;

    m_use_ai_model = _use_ai_model;
    m_alarm_suppression = false;

    m_idx = _idx;
    m_timestamp = _timestamp;
    m_timecount++;
    int res = 0;
    int alarm = 0;
    if (_idx * 64 >= MAX_LENGTH || (_cable_idx < 0 || _cable_idx > 3)) //数据异常
    {
        return -2; // index out of max range
    }

    // processAbnormalData(_data);     // 异常处理，-95的附近出现异常的43的数据，此处做处理，取消掉，另做策略，因为可能不只是43

    res = updateBackgroundTemperature(_data, _idx); // 本di构建
    if (res < 0)
    {
        return res;
    }

    // 等待大概4分钟，等本底值更新几次后，再开始报警逻辑
    if (m_timecount <= TIME_RUN_CNT_TH)
    {
        return -3;
    }

    // 清空报警状态
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        m_alarm_status[i] = 0;
        m_alarm_val[i] = 0;
    }

    // 国标定温报警
    if (alarm_gb_switch_flag)
    {
        res = alarmGB(_data, m_alarm_status, m_alarm_val, _idx);
        if (res > 0) alarm = 1;
    }

    // 计算当前值与本底值的温度差，由于一次只传入64个点的数据，其他点差值为0
    for (int i = 0; i < MAX_LENGTH; i++)
    {
        m_sbtract_background[i] = 0;
    }
    calSbtractBackground(m_sbtract_background, _data, _idx);

    // 整体升温时报警抑制                                   //早上存在大面积升温，导致异常报警。此处通过该逻辑处理
    static int suppression_cnt = 0;
    if (m_alarm_suppression)
    {
        suppression_cnt = 60;
    }
    else
    {
        if (suppression_cnt > 0) suppression_cnt--;
    }
    if (suppression_cnt > 0) return -88;

    // 寻找差值数据的波峰
    res = findPeaks(m_sbtract_background, FIND_PEAK_WINDOW_SIZE, FIND_PEAK_TEMP_TH);
    if (res > 0) LOGD("[DEBUG] peak_idx.size(): %d, peak:%d\r\n", result_peak_count, result_peak_info[0].index);

    // 检测与跟踪算法的关键参数----拱形(arch)
    detactArch(_data, _idx, m_sbtract_background, MAX_LENGTH, FIND_PEAK_WINDOW_SIZE);

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

    // 错误点抑制
    unanormalPointSuppression(m_current_temperatures, m_alarm_status, m_alarm_val, _idx, m_alarm_suppression); //出现-95 等异常点通过此处处理，消除报警。

    if (alarm > 0)
    {
        return alarm;
    }
    else
    {
        return res;
    }
}
