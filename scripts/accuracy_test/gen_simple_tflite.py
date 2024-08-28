import os
import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers

def gen_simple_tflite():
    # 定义一个简单的卷积神经网络模型
    def create_conv_model():
        model = keras.Sequential([
            layers.Conv2D(32, (3, 3), activation='relu', input_shape=(28, 28, 1)),
            layers.MaxPooling2D((2, 2)),
            layers.Conv2D(64, (3, 3), activation='relu'),
            layers.MaxPooling2D((2, 2)),
            layers.Flatten(),
            layers.Dense(64, activation='relu'),
            layers.Dense(10, activation='softmax')
        ])
        return model

    # 创建模型
    model = create_conv_model()

    # 编译模型
    model.compile(optimizer='adam',
                  loss='sparse_categorical_crossentropy',
                  metrics=['accuracy'])

    # 查看模型架构
    model.summary()

def gen_simple_tflite2():
    keras_model = keras.Sequential([
        layers.Conv2D(64, (3, 3), activation=None, input_shape=(32, 32, 1), use_bias=False),
        layers.Conv2D(1, (1, 1), activation=None, use_bias=False),
    ])

    # kernel = [[[1, 0, 0],
    #            [0, 1, 0],
    #            [0, 0, 1]]]
    # keras_model.layers[0].set_weights([np.array(kernel).reshape(3, 3, 1, 1)])
    # keras_model.layers[1].set_weights([np.array([[[[1]]]]).reshape(1, 1, 1, 1)])

    keras_model.summary()
    keras_model.trainable = False
    dst_dir = '../results/simple-model'
    os.makedirs(dst_dir, exist_ok=True)

    # gen tflite model
    weight_quant = False
    int8_model = False
    converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS, tf.lite.OpsSet.SELECT_TF_OPS]
    if weight_quant or int8_model:
        converter.experimental_new_converter = True
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        if int8_model:
            converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8, tf.lite.OpsSet.SELECT_TF_OPS]
            converter.target_spec.supported_types = []
            converter.inference_input_type = tf.int8
            converter.inference_output_type = tf.float32
    tflite_model = converter.convert()
    tflite_model_out_path = os.path.join(dst_dir, 'tfl20240807.tflite')
    with open(tflite_model_out_path, "wb") as fp:
        fp.write(tflite_model)
    print('over')


if __name__ == '__main__':
    gen_simple_tflite2()
