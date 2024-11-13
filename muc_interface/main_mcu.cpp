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

std::vector<std::vector<std::string>> readCSV(const std::string &filename) {
    // std::string filename2("D:\\data\\temperature_sensing_cable_V0\\data\\cable\\data_v2\\隧道火灾实验数据\\１、紧急停车带火灾实验\\6.25紧急停车带 1L汽油 线缆参数修改（18℃启开始差温检测）\\二代线 27s报警 20240625.CSV");
    std::ifstream file(filename);

    std::vector<std::vector<std::string>> data;
    if (!file.is_open()) {
        std::cerr << "open error !!!!" << std::endl;
        return data;
    }
    std::string line;

    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row);
    }
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
    // CableTemDet cable_tem_detector;
    // cable_tem_detector.init(control_flag);
    AlarmInfo alarmList[ALARM_INFO_MAX_NUM];
    alg_cable_temperature_detector_init(control_flag);

    string filepath = "../data/test_index.txt";
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "open error" << std::endl;
        return 1;
    }
    std::vector<std::string> vec;
    std::string line;
    while (std::getline(file, line)) {
        vec.push_back(line);
    }
    file.close();

    string csv_data_path = "/home/manu/tmp/cable_demo_parser_save/heta-cable_00000002333232373739393630353737_1_1_segment_0.csv";
    std::vector<std::vector<std::string>> data_raw = readCSV(csv_data_path);

    int res = 0;
    for (int i = 0; i < data_raw.size(); i++) {
        std::vector<std::string> one_line_data = data_raw[i];
        int8_t data[RAW_DATA_ONE_GROUP_LENGTH] = {0};
        int idx;
        int cable_idx = 0;
        std::string group_id_tmp = one_line_data[4];
        std::string time_str = one_line_data[1];
        group_id_tmp.erase(std::remove_if(group_id_tmp.begin(), group_id_tmp.end(), ::isspace), group_id_tmp.end());
        int a = group_id_tmp.size();

        std::string group_id2 = group_id_tmp.substr(1, group_id_tmp.size() - 2);
        idx = stoi(group_id2);
        std::string target = "ALARM: ";

        const std::string time_str_min = "09:00:00.00";
        const std::string time_str_max = "10:00:00.00";
        int totalSecondsMin = convertTimeStringToSeconds(time_str_min);
        int totalSecondsMax = convertTimeStringToSeconds(time_str_max);
        int totalSeconds = convertTimeStringToSeconds(time_str);

//        std::cout << "Total integer seconds: " << totalSeconds << std::endl;
//        std::cout << "Total integer min seconds: " << totalSecondsMin << std::endl;

        if (totalSeconds < totalSecondsMin) {
//            std::cout << "skipping ..." << std::endl;
            continue;
        }

        if (totalSeconds > totalSecondsMax) break;
        std::cout << "time_str -->" << time_str << std::endl;

        for (int j = 0; j < RAW_DATA_ONE_GROUP_LENGTH; j++) {
            size_t pos = one_line_data[6 + j].find(target);
            if (pos != std::string::npos) {
                one_line_data[6 + j].replace(pos, target.length(), "");
            }
            data[j] = stoi(one_line_data[6 + j]);
        }

        int8_t use_ai_model = 0;
        if (idx == 8)
        {
            // printf("process :%d, group:%s \n", i, group_id_tmp.c_str());
            res = alg_cable_temperature_detector_run(data, idx * 64, cable_idx, totalSeconds, alarmList, use_ai_model);
            //printf("********* [[%d]], %s %s, group:%s, res: %d *********\n", i, one_line_data[0].c_str(), one_line_data[1].c_str(), group_id_tmp.c_str(), res);
            if (res > 0) {
                for (int j = 0; j < res; j++) {
                    printf("********* [[%d]], group:%s, res: %d, alarm_type:%d, alarm_temp: %d, addr:%d *********\n", i,
                           group_id_tmp.c_str(), res, alarmList[j].alarm_type, alarmList[j].alarm_temp,
                           alarmList[j].addr, 0);
                }
            }
//            printf("process :%d, res:%d \n", i, res);
        }
//        if (res > 0) break;
    }
    printf("program over");
    return 0;
}