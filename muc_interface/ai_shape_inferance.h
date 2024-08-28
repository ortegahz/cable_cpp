#ifndef __AI_SHAPE_INFERENCE_H__
#define __AI_SHAPE_INFERENCE_H__

#if defined(_WIN32)
#include <stdio.h>
#include <stdlib.h>
#include <tensorflow/lite/c/c_api.h>
#elif defined(__arm__)

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
    int init(void);
    int run(float *_data, float *_output_data);
private:
    TfLiteInterpreter* m_interpreter;
    TfLiteTensor* m_input_tensor;
    const TfLiteTensor* m_output_tensor;
};

#endif // end define __AI_SHAPE_INFERENCE_H__

