# AI感温线缆

ai temperature sensing cable

## 注意
在板子上调试烧写时，要从0x8010000开始烧写，有两处地方要注意：
```bash
1: setting -> target -> IROM
2: setting -> Debug -> use setting -> flash downlowd -> address range
```

## 模型移植文档需求
1、模型onnx格式；
2、模型输入输出，包括尺寸、通道数、数据值的范围，padding方式，padding理论填充值，前处理方式，后处理方式等；
3、可以直接验证的python代码；
4、验证集数据，数量越多越好；
5、其他特别要注意的地方，比如说是输入是当前值还是差值等；
