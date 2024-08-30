# 调试移植

## 说明
1. 为了移植方便，先进行了pc端调试，然后将代码直接拷贝到mcu工程中即可；
2. c/c++代码都加入了条件编译；
3. pc端调试工程使用cmake构建；

## pc端调试
1. 下载代码
```bash
git clone xxx
cd AITempCablePcDebug/muc_interface
```
2. 构建工程
```bash
mkdir build
cd build
cmake ..
make
```
3. 运行
```bash
cp ../tflite-2.3.1-lib/lib/tensorflowlite_c.dll ./
mcu.exe
```
### clion调试
推荐用clion调试，直接打开AITempCablePcDebug/muc_interface即可，方便省心不要管cmake、make和编译工具链；

## 移植
1. 下载嵌入式工程
```bash
# http://gerrit.jbufa.com/admin/repos/ai_cable_detect/temperature-sensitive,general
git clone "ssh://weixing@gerrit.jbufa.com:29418/ai_cable_detect/temperature-sensitive" && (cd "temperature-sensitive" && scp -p -O -P 29418 $(git config user.name)@gerrit.jbufa.com:hooks/commit-msg $(git rev-parse --git-dir)/hooks/)
```
2. 拷贝代码
```bash
cp ai_shape_inference.h temperature-sensitive/Project/Algorithm
cp ai_shape_inference_mcu.cpp temperature-sensitive/Project/Algorithm
cp alg_cable_tem_detector.h temperature-sensitive/Project/Algorithm
cp alg_cable_tem_detector.cpp temperature-sensitive/Project/Algorithm
cp alg_cable_tem_detector_interface.cpp temperature-sensitive/Project/Algorithm
cp alg_cable_tem_detector_interface.h temperature-sensitive/Project/Algorithm
```
总共6个文件
3. 拷贝EdgeAI库
```bash
cp EdgeAI temperature-sensitive/Project
```
EdgeAI文件夹的生成参考[README.md](scripts/README.md)，里面是模型和gd32-cortexM4 AI库；
4. 运行工程，使用keil打开temperature-sensitive/Project/MDK-ARM/Project.uvprojx

**注意**：在板子上调试烧写时，要从0x8010000开始烧写，有两处地方要注意：
```bash
1: setting -> target -> IROM
2: setting -> Debug -> use setting -> flash downlowd -> address range
```

### 文件说明
alg_cable_tem_detector_interface是对外接口，只有两个函数init/run；

main_mcu.cpp调试主程序入口；

## misc
现成的tflite windows下编译好的库(https://discuss.tf.wiki/t/topic/1686)


