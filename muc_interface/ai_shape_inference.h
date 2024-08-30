#ifndef __AI_SHAPE_INFERENCE_H__
#define __AI_SHAPE_INFERENCE_H__

#if defined(_WIN32)
#include <stdio.h>
#include <stdlib.h>
#include <tensorflow/lite/c/c_api.h>
#elif defined(__arm__)
#include "shapemodel.h"
#include "shapemodel_data.h"
#endif

#define SHAPE_MODEL_INPUT_WIDTH 32
#define SHAPE_MODEL_INPUT_HEIGHT 16
#define SHAPE_MODEL_INPUT_CHANNEL 1
#define SHAPE_MODEL_INPUT_SIZE 512
#define SHAPE_MODEL_INPUT_SIZE_BYTES 2048
#define SHAPE_MODEL_OUTPUT_WIDTH 1
#define SHAPE_MODEL_OUTPUT_HEIGHT 1
#define SHAPE_MODEL_OUTPUT_CHANNEL 2
#define SHAPE_MODEL_OUTPUT_SIZE 2
#define SHAPE_MODEL_OUTPUT_SIZE_BYTES 8
#define SHAPE_MODEL_CLASS_NUM SHAPE_MODEL_OUTPUT_CHANNEL


class AiShapeModel
{
public:
    AiShapeModel(){};
    ~AiShapeModel(){};
    int init();
    int run(float *_data, float *_output_data);
private:
    bool init_flag = false;
#if defined(_WIN32)
    TfLiteInterpreter* m_interpreter;
    TfLiteTensor* m_input_tensor;
    const TfLiteTensor* m_output_tensor;
#elif defined(__arm__)
    ai_handle m_interpreter;
    ai_buffer * m_input_tensor;
    ai_buffer * m_output_tensor;
    float *m_input_data;
    float *m_output_data;
    AI_ALIGNED(32) ai_u8 m_activations[AI_SHAPEMODEL_DATA_ACTIVATIONS_SIZE];
#endif
};

#endif // end define __AI_SHAPE_INFERENCE_H__
