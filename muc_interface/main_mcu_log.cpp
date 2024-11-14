#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

#include "alg_cable_tem_detector.h"
#include "alg_cable_tem_detector_interface.h"

#define RAW_DATA_ONE_GROUP_LENGTH 64

using namespace std;

std::vector<std::vector<std::string>> readLog(const std::string &filename) {
    std::ifstream file(filename);
    std::vector<std::vector<std::string>> data;

    if (!file.is_open()) {
        std::cerr << "Open error !!!!" << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row;

        // 去掉括号和逗号，方便解析
        std::replace(line.begin(), line.end(), '[', ' ');
        std::replace(line.begin(), line.end(), ']', ' ');
        std::replace(line.begin(), line.end(), ',', ' ');

        std::stringstream ss(line);
        std::string cell;

        // 提取时间、ID和范围
        std::string time, id0, id1, dummy;
        ss >> time >> id0 >> id1 >> dummy;
        row.push_back(time);
        row.push_back(id0);
        row.push_back(id1);

        // 提取起始和结束索引
        size_t dot_pos = dummy.find('.');
        size_t tilde_pos = dummy.find('~');
        if (dot_pos != std::string::npos && tilde_pos != std::string::npos) {
            std::string start_idx = dummy.substr(dot_pos + 3, tilde_pos - (dot_pos + 3)); // 跳过 '1.1.'
            std::string end_idx = dummy.substr(tilde_pos + 1);
            row.push_back(start_idx);
            row.push_back(end_idx);
        }

        // 提取温度数据
        while (ss >> cell) {
            row.push_back(cell);
        }

        data.push_back(row);
    }

    file.close();
    return data;
}

int convertTimeStringToSeconds(const std::string &timeStr) {
    int hours, minutes, seconds;
    int microseconds = 0;
    char delim;
    std::istringstream iss(timeStr);
    iss >> hours >> delim >> minutes >> delim >> seconds;
    if (iss.peek() == '.') {
        iss >> delim >> microseconds;
    }
    double totalSeconds = hours * 3600.0 + minutes * 60.0 + seconds + microseconds / 1e6;
    return static_cast<int>(totalSeconds);
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    printf("%s\n", "开始");
    cout << "开始" << endl;

    int control_flag = 0b111;
    // 初始化温度检测器
    // cable_tem_detector.init(control_flag);
    AlarmInfo alarmList[ALARM_INFO_MAX_NUM];
    alg_cable_temperature_detector_init(control_flag);

    string log_data_path = "/home/manu/tmp/split_logs/heta-cable_00000002333232373739393630353737_1_1.log";
    std::vector<std::vector<std::string>> data_raw = readLog(log_data_path);

    int res = 0;
    for (int i = 0; i < data_raw.size(); i++) {
        std::vector<std::string> one_line_data = data_raw[i];
        int8_t data[RAW_DATA_ONE_GROUP_LENGTH] = {0};

        // 提取时间和起始索引
        std::string time_str = one_line_data[0];
        int start_idx = stoi(one_line_data[3]); // start_idx 从解析后数据中获得

        // 提取温度数据
        for (int j = 5; j < one_line_data.size(); j++) {
            data[j - 5] = stoi(one_line_data[j]); // 调整索引
        }

        int totalSeconds = convertTimeStringToSeconds(time_str);
        const std::string time_str_min = "00:00:00.00";
        const std::string time_str_max = "23:40:00.00";
        int totalSecondsMin = convertTimeStringToSeconds(time_str_min);
        int totalSecondsMax = convertTimeStringToSeconds(time_str_max);

        if (totalSeconds < totalSecondsMin) {
//            std::cout << "skipping ..." << std::endl;
            continue;
        }

        if (totalSeconds > totalSecondsMax) break;
//        std::cout << "time_str -->" << time_str << std::endl;

        std::cout << "time_str -->" << time_str << std::endl;
        std::cout << "totalSeconds -->" << totalSeconds << std::endl;

        // 调用温度检测函数，传入start_idx
        int8_t use_ai_model = 0;
//        res = alg_cable_temperature_detector_run(data, start_idx, 0, totalSeconds, alarmList, use_ai_model);
        std::cout << "start_idx -->" << start_idx << std::endl;
        res = alg_cable_temperature_detector_run_all(data, start_idx, 0, totalSeconds, alarmList, use_ai_model);

        // 打印报警信息
        if (res > 0) {
            for (int j = 0; j < res; j++) {
                printf("********* [[%d]], start_idx:%d, res: %d, alarm_type:%d, alarm_temp: %d, addr:%d ** *******\n",
                       i,
                       start_idx, res, alarmList[j].alarm_type, alarmList[j].alarm_temp,
                       alarmList[j].addr, 0);
            }
        }
    }

//    printf("program over");
    return 0;

}