#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include "ai_shape_inference.h"


using namespace std;


int main(int argc, char *argv[])
{
    setbuf(stdout,NULL);
    printf("program %s\n", "start");

    AiShapeModel model;
    model.init();
    int res;
    float data[32];
    for (int idx = 0; idx < 3; idx++)
    {
        float input_data[SHAPE_MODEL_INPUT_SIZE] = {0.0f}; // Adjust size and content based on your model
        for (int i = 0; i < SHAPE_MODEL_INPUT_HEIGHT; i++)
        {
            for (int j = 0; j < SHAPE_MODEL_INPUT_WIDTH; j++)
            {
                input_data[i*SHAPE_MODEL_INPUT_WIDTH + j] = i + j + idx;
            }
        }
        float output_data[2];
        res = model.run(input_data, output_data);
        printf("Output pred:%d, [%d]: %f, [%d]: %f\n", res, 0, output_data[0], 1, output_data[1]);
    }

    printf("program %s\n", "end");
    return 0;
}