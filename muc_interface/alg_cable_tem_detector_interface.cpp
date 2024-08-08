#include "alg_cable_tem_detector_interface.h"

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

class CableTemDetTest
{
public:
    CableTemDetTest(){};
    ~CableTemDetTest(){};
    int init(int _control){return 0;};
    int run(int *_data, int _idx, int _cable_idx){return 0;};
};

CableTemDetTest det;
int alg_cable_temperature_detector_init(int _control)
{
    printf("CableTemDetTest init");
    det.init(_control);
    std::vector<std::pair<int, float>> peaks_info;
    peaks_info.push_back(std::make_pair(1, 2.0));
    peaks_info.push_back(std::make_pair(2, 3.0));
    std::sort(peaks_info.begin(), peaks_info.end(), [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
        return a.second > b.second;
    });
    return 0;
}

int alg_cable_temperature_detector_run(int *_data, int _idx, int _cable_idx, char *timestamp)
{
    printf("CableTemDetTest run");
    det.run(_data, _idx, _cable_idx);
    return 0;
}
