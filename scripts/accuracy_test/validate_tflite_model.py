import numpy as np
import tensorflow as tf

def validate_tflite_model():
    model_path = '../results/simple-model/tfl20240807.tflite'
    interpreter = tf.lite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    x = [[[1, 0, 0],
           [0, 1, 0],
           [0, 1, 0]]]
    x = np.array(x, dtype=np.float32)
    x = np.transpose(x, axes=(1, 2, 0))
    x = np.expand_dims(x, axis=(0))
    interpreter.set_tensor(input_details[0]['index'], x)
    interpreter.invoke()
    output_data = interpreter.get_tensor(output_details[0]['index'])
    print("Predicted class:", output_data)

def validate_tflite_model_Tangxiangyu():
    model_path = 'D:/data/02_CableTemperatureDetector/fromTang_20240827/best_model_noview_20240828.tflite'
    interpreter = tf.lite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()

    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    x = list(range(32))
    x = [[[j+i for j in x] for i in range(16)]]
    x = np.array(x, dtype=np.float32)
    x = np.transpose(x, axes=(1, 2, 0))
    x = np.expand_dims(x, axis=(0))
    interpreter.set_tensor(input_details[0]['index'], x)
    interpreter.invoke()
    output_data = interpreter.get_tensor(output_details[0]['index'])
    print("Predicted class:", output_data)

if __name__ == '__main__':
    validate_tflite_model_Tangxiangyu()
