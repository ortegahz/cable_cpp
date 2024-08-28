## 模型转换

### onnx转tflite
```bash
python onnx2tflite.py
```

### 导出EdgeAI
利用xcubeai将tflite模型转成可以集成到keil工程里的EdgeAI代码和库
```bash
python deploy.py
```
然后会看到下面输出
```bash
{'config': 'results/mobilenetv2_100.yaml', 'model_path': 'modelzoo/food-101-224/model_best.pth.tar', 'convert_type': 1, 'tflite_val_path': '../../../datasets/food-101/validation', 'c_project_path': 'results/t2', 'stm32cubeai_path': 'C:/Program1/xcubeai/stedgeai-windows-9.0.0', 'series': 'f4', 'eval': False, 'compiler': 1, 'img_size': None, 'separation': 0, 'separation_scale': 2, 'class_name': 'no,fire'}
Command:
C:/Program1/xcubeai/stedgeai-windows-9.0.0\Utilities\windows\stedgeai.exe generate --name network_1 -m results/mobilenetv2_100.tflite --type tflite --compression none --verbosity 1 --workspace results/t2\temp --output results/t2\Edge_AI\model --allocate-inputs --target stm32l5 --allocate-outputs
will be excuted to generate net codes
Net codes generation successful
over common_deploy
over
```
然后results/xxx文件夹下面会生成一个EdgeAI文件夹，EdgeAI里就是移植要用到的模型和库文件。

**注意**：如果模型只有一层卷积，使用stedgeai.exe工具会报错，又加入一层之后就好了。
```bash
# error 信息
xcudeai TOOL ERROR: min() arg is an empty sequence
```

#
