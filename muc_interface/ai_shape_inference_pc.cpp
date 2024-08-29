#include <math.h>
#include "ai_shape_inference.h"


// Helper function to read a binary file into a buffer
unsigned char* read_file(const char* filename, size_t* out_size) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char* buffer = (unsigned char*)malloc(size);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file);
    fclose(file);

    *out_size = size;
    return buffer;
}

int AiShapeModel::init()
{
    if (init_flag)
    {
        printf("AiShapeModel::init already\r\n");
        return 1;
    }

    const char* model_path = "D:/data/02_CableTemperatureDetector/fromTang_20240827/best_model_noview_20240828.tflite";
    size_t model_size;
    unsigned char* model_data = read_file(model_path, &model_size);
    if (model_data == NULL) return -1;

    // Load the model
    TfLiteModel* model = TfLiteModelCreate(model_data, model_size);
    if (model == NULL) {
        fprintf(stderr, "Failed to load model\n");
        free(model_data);
        return -1;
    }

    // Create the interpreter
    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    m_interpreter = TfLiteInterpreterCreate(model, options);
    TfLiteInterpreterOptionsDelete(options);
    TfLiteModelDelete(model);

    if (m_interpreter == NULL) {
        fprintf(stderr, "Failed to create interpreter\n");
        free(model_data);
        return -1;
    }

    // Allocate tensors
    if (TfLiteInterpreterAllocateTensors(m_interpreter) != kTfLiteOk) {
        fprintf(stderr, "Failed to allocate tensors\n");
        TfLiteInterpreterDelete(m_interpreter);
        free(model_data);
        return -1;
    }

    // Get input tensor
    m_input_tensor = TfLiteInterpreterGetInputTensor(m_interpreter, 0);
    if (m_input_tensor == NULL) {
        fprintf(stderr, "Failed to get input tensor\n");
        TfLiteInterpreterDelete(m_interpreter);
        free(model_data);
        return -1;
    }

    // Get output tensor
    m_output_tensor = TfLiteInterpreterGetOutputTensor(m_interpreter, 0);

    // 不要free！！！
    // free(model_data);

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
    // Fill input tensor with data
    if (TfLiteTensorCopyFromBuffer(m_input_tensor, _data, SHAPE_MODEL_INPUT_SIZE_BYTES) != kTfLiteOk) {
        fprintf(stderr, "Failed to copy data to input tensor\n");
        TfLiteInterpreterDelete(m_interpreter);
        return -1;
    }

    // Run inference
    if (TfLiteInterpreterInvoke(m_interpreter) != kTfLiteOk) {
        fprintf(stderr, "Failed to invoke interpreter\n");
        TfLiteInterpreterDelete(m_interpreter);
        return -1;
    }

    // Get output data from output tensor
    float tmp_output_data[SHAPE_MODEL_CLASS_NUM] = {0.0f};
    if (TfLiteTensorCopyToBuffer(m_output_tensor, tmp_output_data, SHAPE_MODEL_OUTPUT_SIZE_BYTES) != kTfLiteOk) {
        fprintf(stderr, "Failed to copy data from output tensor\n");
        TfLiteInterpreterDelete(m_interpreter);
        return -1;
    }

    int res;
    softmax(tmp_output_data, _output_data, SHAPE_MODEL_CLASS_NUM);
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