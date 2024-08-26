import os
import cv2
import numpy as np
import torch
import timm
from torchvision import transforms
from timm.utils.model import reparameterize_model
from timm.utils.onnx import onnx_export
import onnx
import onnxruntime as ort
from onnxsim import simplify
from pprint import pprint


def t1():
    m = timm.create_model('mobilenetv2_100', pretrained=True)
    m.eval()

def t2():
    model_names = timm.list_models(pretrained=True)
    pprint(model_names)

def t3():
    # m = timm.create_model('resnet50', pretrained=True)#, num_classes=0, global_pool='')
    m = timm.create_model('mobilenetv2_100', pretrained=True)
    o = m(torch.randn(2, 3, 224, 224))
    print(f'Unpooled shape: {o.shape}')
    print(o)
    print('over')

def t4():
    model = timm.create_model('mobilenetv2_100', pretrained=True)
    model.eval()
    image_path = "D:/temp/dog1.jpg"
    image = cv2.imread(image_path)
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

    preprocess = transforms.Compose([
        transforms.ToPILImage(),
        transforms.Resize(256),
        transforms.CenterCrop(224),
        transforms.ToTensor(),
        # transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225]),
    ])
    input_tensor = preprocess(image)
    input_batch = input_tensor.unsqueeze(0)

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print("device: ", device)
    model.to(device)
    input_batch = input_batch.to(device)

    with torch.no_grad():
        output = model(input_batch)

    # 结果处理
    probabilities = torch.nn.functional.softmax(output[0], dim=0)

    # 获取 ImageNet 类别标签
    # url = "https://raw.githubusercontent.com/pytorch/hub/master/imagenet_classes.txt"
    classes = [line.strip() for line in open("imagenet_classes_ch.txt").readlines()]

    # 显示 top 5 预测结果
    top5_prob, top5_catid = torch.topk(probabilities, 5)
    for i in range(top5_prob.size(0)):
        print(classes[top5_catid[i]], top5_prob[i].item())

    save_dir = "results"
    os.makedirs(save_dir, exist_ok=True)
    save_path = save_dir + '/mobilenetv2_100.pth'  # 你想要保存模型的路径
    torch.save(model.state_dict(), save_path)
    print('over')

def onnx_export_example():
    model_name = "mobilenetv2_100"
    model = timm.create_model(model_name, pretrained=True)
    reparam_flag = False
    if reparam_flag:
        model = reparameterize_model(model)

    output = "results/{}_simplily.onnx".format(model_name)
    input_size = (3, 224, 224)
    onnx_export(
        model,
        output,
        opset=None,
        dynamic_size=False,
        aten_fallback=False,
        keep_initializers=False,
        check_forward=False,
        training=False,
        verbose=False,
        use_dynamo=False,
        input_size=input_size,
        batch_size=1,
    )
    print('over')

def onnx_inference():
    image_path = "D:/temp/dog1.jpg"
    image = cv2.imread(image_path)
    model_name = "mobilenetv2_100"
    model_path = "results/{}.onnx".format(model_name)
    sess = ort.InferenceSession(model_path)
    input_name = sess.get_inputs()[0].name
    print('sess.get_inputs()[0]', sess.get_inputs()[0])
    input_shape = sess.get_inputs()[0].shape
    output_name = sess.get_outputs()[0].name

    img = cv2.resize(image, (input_shape[3], input_shape[2]))
    img = img[:, :, ::-1]
    img = img.astype(np.float32)
    img = img / 255.0

    def numpy_normalize(image, mean, std, type):
        image = np.array(image, dtype=np.float32)
        if type == 'hwc':
            for i in range(image.shape[2]):
                image[:, :, i] = (image[:, :, i] - mean[i]) / std[i]
        return image
    # img = (img - np.array([0.485, 0.456, 0.406])) / np.array([0.229, 0.224, 0.225])
    img = numpy_normalize(img, [0.485, 0.456, 0.406], [0.229, 0.224, 0.225], 'hwc')
    img = np.transpose(img, axes=(2, 0, 1))
    img = np.expand_dims(img, axis=0)
    result = sess.run([output_name], {input_name: img})
    # print(result)

    probabilities = result[0][0]
    def softmax(x):
        # 减去最大值以防止指数计算中的溢出
        e_x = np.exp(x - np.max(x))
        return e_x / e_x.sum(axis=-1, keepdims=True)
    probabilities = softmax(probabilities)
    classes = [line.strip() for line in open("imagenet_classes_ch.txt").readlines()]

    def topk_numpy(probabilities, k):
        # 获取前 k 个最大值的索引
        indices = np.argpartition(probabilities, -k)[-k:]
        # 获取前 k 个最大值
        topk_values = probabilities[indices]
        # 对前 k 个最大值排序
        sorted_indices = np.argsort(-topk_values)
        topk_values = topk_values[sorted_indices]
        topk_indices = indices[sorted_indices]
        return topk_values, topk_indices

    top5_prob, top5_catid = topk_numpy(probabilities, 5)
    for i in range(top5_prob.size):
        print(classes[top5_catid[i]], top5_prob[i].item())
    print('over')

def onnx_simplify():
    model = onnx.load('results/mobilenetv2_100.onnx')
    model_simp, check = simplify(model)
    assert check, "Simplified ONNX model could not be validated"
    onnx.save(model_simp, 'results/mobilenetv2_100_simplily.onnx')
    # 实际测试感觉simplify用处不大，可以不加，或者手动分析，手动改掉不合理的地方。
    print('over')

if __name__ == '__main__':
    # t4()
    # onnx_export_example()
    # onnx_inference()
    onnx_simplify()
