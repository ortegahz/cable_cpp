#include <stdio.h>
#include <math.h>
#include <string.h>
#include <rtthread.h>
#include "ai_shape_inference.h"


int AiShapeModel::init()
{
    if (init_flag)
    {
        //printf("AiShapeModel::init already\r\n");
        return 1;
    }

    ai_error err1,err2;
    const ai_handle act_addr[] = { m_activations };
    err1 = ai_shapemodel_create_and_init(&m_interpreter, act_addr, NULL);
    if (err1.type != AI_ERROR_NONE) {
        return -1;
    }

    m_input_tensor = ai_shapemodel_inputs_get(m_interpreter, NULL);
    m_output_tensor = ai_shapemodel_outputs_get(m_interpreter, NULL);
    m_input_data = (float *)m_input_tensor[0].data;
    m_output_data = (float *)m_output_tensor[0].data;

    init_flag = true;

    return 0;
}

void softmax(const float* input, float* output, int length) {
    // 计算输入向量的最大值，用于数值稳定性
    float max_input = input[0];
    for (int i = 1; i < length; ++i) {
        if (input[i] > max_input) {
            max_input = input[i];
        }
    }

    // 计算所有输入的指数并累加
    float sum_exp = 0.0f;
    for (int i = 0; i < length; ++i) {
        output[i] = expf(input[i] - max_input);  // 减去最大值以提高数值稳定性
        sum_exp += output[i];
    }

    // 归一化输出
    for (int i = 0; i < length; ++i) {
        output[i] /= sum_exp;
    }
}

int AiShapeModel::run(float *_data, float *_output_data)
{
    memcpy(m_input_data, _data, SHAPE_MODEL_INPUT_SIZE_BYTES);
    m_input_tensor[0].data = AI_HANDLE_PTR(m_input_data);
    m_output_tensor[0].data = AI_HANDLE_PTR(m_output_data);

    ai_i32 batch;
    batch = ai_shapemodel_run(m_interpreter, m_input_tensor, m_output_tensor);
    if (batch != 1) {
        return -1;
    }
    rt_kprintf("Output pred: [%d]: %f, [%d]: %f\r\n", 0, m_output_data[0], 1, m_output_data[1]);

    int res;
    softmax(m_output_data, _output_data, SHAPE_MODEL_CLASS_NUM);
    if (_output_data[0] > _output_data[1])
    {
        res = 0;
    }
    else
    {
        res = 1;
    }
    return res;
}
