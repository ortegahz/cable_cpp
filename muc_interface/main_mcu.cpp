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

int constructInputData()
{
    return 0;
}

std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
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

int main(int argc, char *argv[])
{
    setbuf(stdout,NULL);
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
    while (std::getline(file, line)) { // 读取文件每一行
        vec.push_back(line); // 将每一行添加到向量中
    }
    file.close();

    // string filepath = "D:/data/temperature_sensing_cable_V0/data/cable/data_v2/隧道火灾实验数据/１、紧急停车带火灾实验/6.22紧急停车带隧道火灾实验数据/6.22实验 1L汽油 紧急停车带 风速0.3－43S报警/20240622 保护套线原始数据.CSV";
    string csv_data_path = "../data/test.CSV";
    std::vector<std::vector<std::string>> data_raw = readCSV(csv_data_path);

    int res = 0;
    for (int i = 0; i < data_raw.size(); i++)
    {
        std::vector<std::string> one_line_data = data_raw[i];
        int8_t data[RAW_DATA_ONE_GROUP_LENGTH] = {0};
        int idx;
        int cable_idx = 0;
        std::string group_id_tmp = one_line_data[4];
        group_id_tmp.erase(std::remove_if(group_id_tmp.begin(), group_id_tmp.end(), ::isspace), group_id_tmp.end());
        int a = group_id_tmp.size();
        std::string group_id2 = group_id_tmp.substr(1, group_id_tmp.size()-2);
        idx = stoi(group_id2);
        std::string target = "ALARM: ";
        for (int j = 0;j < RAW_DATA_ONE_GROUP_LENGTH; j++)
        {
            size_t pos = one_line_data[6+j].find(target);
            if (pos != std::string::npos)
            {
                one_line_data[6+j].replace(pos, target.length(), "");
            }
            data[j] = stoi(one_line_data[6+j]);
        }
        if (i == 80)
        {
            int a = 0;
            data[32] = 103;
        }
        // if (idx == 0)
        {
            // printf("process :%d, group:%s \n", i, group_id_tmp.c_str());
            res = alg_cable_temperature_detector_run(data, idx*64, cable_idx, i, alarmList);
            //printf("********* [[%d]], %s %s, group:%s, res: %d *********\n", i, one_line_data[0].c_str(), one_line_data[1].c_str(), group_id_tmp.c_str(), res);
            if (res > 0)
            {
                for (int j = 0; j < res; j++)
                {
                    // printf("********* [[%d]], group:%s, res: %d, %d, %d *********\n", i, group_id_tmp.c_str(), alarmList[j].alarm_type, alarmList[j].alarm_temp, alarmList[j].addr);
                }
            }
            printf("process :%d, res:%d \n", i, res);
        }
        // if (i > 256) break;
    }
    printf("program over");
    return 0;
}