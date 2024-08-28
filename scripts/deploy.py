import os
import re
import yaml
import argparse
import shutil
import sys
import subprocess
import warnings
from functools import partial
import numpy as np
import torch
import torch.nn as nn
import onnxruntime as ort
import tensorflow as tf


class tfOrtModelRuner():
    def __init__(self, model_path: str):
        if model_path.endswith(".tflite"):
            self.model_interpreter = tf.lite.Interpreter(model_path=model_path)
            self.model_interpreter.allocate_tensors()
            self.model_input_details = self.model_interpreter.get_input_details()[0]
            self.model_output_details = self.model_interpreter.get_output_details()
            self.model_type = 1
        else:
            self.ort_sess = ort.InferenceSession(model_path)
            self.model_type = 0

    def __call__(self, x):
        if self.model_type == 0:
            return self.ort_sess.run(None, {'input.1': x})[0]
        else:
            self.model_interpreter.set_tensor(self.model_input_details['index'], x)
            self.model_interpreter.invoke()
            out_list = []
            for output_details in self.model_output_details:
                out_list.append(self.model_interpreter.get_tensor(output_details['index']))
            if len(out_list) == 1:
                return out_list[0]
            else:
                return out_list
class ClassifierTfOrt():
    def __init__(self, cfg, model_path):
        super(ClassifierTfOrt, self).__init__()
        self.separation = 0 # ["separation"]
        self.separation_scale = 2 # cfg["separation_scale"]

        if model_path is None:
            print('model_path is NONE')
        else:
            self.model = tfOrtModelRuner(model_path)
            self.sp = 0
            self.model_type = os.path.splitext(model_path)[-1]
            self.weight, self.bias = self.model.model_input_details["quantization"]
            self.input_size = self.model.model_input_details["shape"][1:]

    def __call__(self, inputs):
        pred_list0 = []
        for x in inputs:
            if self.model_type == ".tflite":
                x = np.clip(x.permute(1, 2, 0).cpu().numpy() / self.weight + self.bias, -128, 127)
                x = x.astype("int8")
                h, w, c = x.shape[:3]
            else:
                x = x.cpu().numpy()
                c, h, w = x.shape[:3]
            if self.sp == 1:
                print('[WARNING!!!]  self.sp == 1')
            else:
                out = self.model(x[None, :, :, :])
            out0 = torch.tensor(out, device=inputs.device)
            pred_list0.append(out0)
        out0 = torch.cat(pred_list0, dim=0)
        return out0

def extract_version_number(text):
    version_pattern = re.compile(r'(\d+(\.\d+){1,2})')
    match = version_pattern.search(text)
    if match:
        return match.group(1)
    return None

def code_replace(opt, line, cfg, tfmodel):
    sp = opt.separation
    spc = opt.separation_scale

    w, b = tfmodel.weight, tfmodel.bias
    mean = [123.675, 116.28, 103.53]
    std  = [58.395, 57.12, 57.375]

    if "SEPARATION_CODE" in line:
        line = f"#define SEPARATION {sp}\n"
    elif "SEPARATION_SCALE_CODE" in line:
        line = f"#define SEPARATION_SCALE {spc}\n"
    elif "FIX_FACTOR0_CODE" in line:
        if sp > 0:
            line = f"#define FIX_FACTOR0 {tfmodel.fix0}f\n"
        else:
            line = ""
    elif "FIX_FACTOR1_CODE" in line:
        if sp > 0:
            line = f"#define FIX_FACTOR1 {tfmodel.fix1}f\n"
        else:
            line = ""
    elif "IMG_NORM_CODE" in line:
        print('w:{}, b:{}'.format(w, b))
        if w == 0:
            w = 1
        line = f"#define IMG_NORM\n" \
               f"#define bias_r {-mean[0]/std[0]/w+b}f\n" \
               f"#define bias_g {-mean[1]/std[1]/w+b}f\n" \
               f"#define bias_b {-mean[2]/std[2]/w+b}f\n" \
               f"#define weight_r {1/std[0]/w}f\n" \
               f"#define weight_g {1/std[1]/w}f\n" \
               f"#define weight_b {1/std[2]/w}f\n"
    elif "ACTIVITIES_CODE" in line:
        # names_path = os.path.join(cfg["data_dir"], "validation")
        # names_ = os.listdir(names_path)
        names_ = opt.class_name.split(',')
        names = []
        for name in names_:
            names.append('"'+name.strip()+'"')
        names = ",".join(names)
        line = "const char *activities[]={"+names+"};\n"
    return line
def common_code_replace(opt, line):
    if "RCU_CODE" in line:
        line = ""
    if "_CODE" in line:
        line = ""
    return line
def gen_common_ic_codes(opt, utils_path, ai_model_path, code_replace, **kwargs):
    with open("c_codes/ai_model.h", "r") as f:
        lines = f.readlines()
    with open(os.path.join(ai_model_path, "ai_model.h"), "w", encoding="utf-8") as f:
        for line in lines:
            line=code_replace(opt,line,**kwargs)
            line = common_code_replace(opt, line)
            if len(line)>0:
                f.write(line)

    with open("c_codes/ai_model.c", "r") as f:
        lines = f.readlines()
    with open(os.path.join(ai_model_path, "ai_model.c"), "w", encoding="utf-8") as f:
        for line in lines:
            line=code_replace(opt,line,**kwargs)
            line = common_code_replace(opt, line)
            if len(line)>0:
                f.write(line)


def copy_stlib(opt, stm32ai_path, stlib_path, version):
    stlib_src_path = os.path.join(stm32ai_path, "Middlewares", "ST", "AI")
    if opt.series == "f4":
        lib_type = "ARMCortexM4"
    elif opt.series == "h7":
        if version < "9.0.0":
            lib_type = "ARMCortexM4"
        else:
            lib_type = "ARMCortexM7"

    elif opt.series == "m33":
        lib_type = "ARMCortexM33"
    else:
        assert False, f"Unsupported target:{opt.series}"

    prefix = "MDK"
    lib_src_path = os.path.join(stlib_src_path, "Lib", prefix, lib_type)
    assert os.path.exists(lib_src_path), f"Stlib path:{lib_src_path}\n do not exist"
    lib_name = os.listdir(lib_src_path)[0]
    lib_src_path = os.path.join(lib_src_path, lib_name)
    lib_dst_path = os.path.join(stlib_path, "Lib", "NetworkRuntime.lib")
    os.makedirs(os.path.join(stlib_path, "Lib"), exist_ok=True)
    shutil.copy(lib_src_path, lib_dst_path)

    prefix = "GCC"
    lib_src_path = os.path.join(stlib_src_path, "Lib", prefix, lib_type)
    assert os.path.exists(lib_src_path), f"Stlib path:{lib_src_path}\n do not exist"
    lib_names = os.listdir(lib_src_path)
    lib_name = None
    for name in lib_names:
        if "PIC" not in name:
            lib_name = name
            break
    assert lib_name is not None, f"Library file not found in folder path :{lib_src_path}"
    lib_src_path = os.path.join(lib_src_path, lib_name)
    prefix = "lib"
    lib_dst_path = os.path.join(stlib_path, "Lib", prefix + "NetworkRuntime.a")
    os.makedirs(os.path.join(stlib_path, "Lib"), exist_ok=True)
    shutil.copy(lib_src_path, lib_dst_path)

    inc_dst_path = os.path.join(stlib_path, "Inc")
    if os.path.exists(inc_dst_path):
        shutil.rmtree(inc_dst_path)
    inc_src_path = os.path.join(stlib_src_path, "Inc")
    shutil.copytree(inc_src_path, inc_dst_path)
    license_src_path = os.path.join(stlib_src_path, "LICENSE.txt")
    license_dst_path = os.path.join(stlib_path, "LICENSE.txt")
    shutil.copy(license_src_path, license_dst_path)

def gen_net_codes(stm32ai_exe_path, model_path, name, work_space_path, output_path, version):
    if version >= "9.0.0":
        param_name = "target"
    else:
        param_name = "series"
    cmd = (f"{stm32ai_exe_path} "
           f"generate "
           f"--name {name} "
           f"-m {model_path} "
           f"--type tflite "
           f"--compression none "
           f"--verbosity 1 "
           f"--workspace {work_space_path} "
           f"--output {output_path} "
           f"--allocate-inputs "
           f"--{param_name} stm32l5 "
           f"--allocate-outputs")
    print(f"Command:\n{cmd}\nwill be excuted to generate net codes")
    result = subprocess.run(cmd, stdout=subprocess.PIPE)
    # result = subprocess.run(['powershell', '-Command', cmd], stdout=subprocess.PIPE)
    # print(result.stdout.decode("utf-8","ignore"))
    if result.returncode != 0:
        print("Net codes generation failed")
        exit(-1)
    print("Net codes generation successful")
def common_deploy(opt, save_path, tflite_model_path, gen_codes_path, gen_ai_model_codes):
    model_path = tflite_model_path
    version = extract_version_number(os.path.split(opt.stm32cubeai_path)[1])
    assert version is not None, f"Unkown X-CUBE-AI version:{version}"
    assert version <= "9.0.0", f"The version of X-CUBE-AI must >= 9.0.0, current is {version}"

    stm32ai_exe_path = os.path.join(opt.stm32cubeai_path, "Utilities", "windows", "stedgeai.exe")
    temp_path = os.path.join(save_path, "temp")
    output_path = os.path.join(gen_codes_path, "Edge_AI")
    if os.path.exists(output_path):
        shutil.rmtree(output_path)
    output_model_path = os.path.join(output_path, "model")
    stlib_path = os.path.join(output_path, "ST_Lib")
    utils_path = os.path.join(output_path, "utils")

    os.makedirs(temp_path, exist_ok=True)
    os.makedirs(output_path, exist_ok=True)
    os.makedirs(output_model_path, exist_ok=True)
    os.makedirs(stlib_path, exist_ok=True)
    os.makedirs(utils_path, exist_ok=True)
    copy_stlib(opt, opt.stm32cubeai_path, stlib_path, version)

    license_src_path = os.path.join(os.path.join(opt.stm32cubeai_path, "Middlewares", "ST", "AI"), "LICENSE.txt")
    license_dst_path = os.path.join(output_model_path, "LICENSE.txt")
    shutil.copy(license_src_path, license_dst_path)

    gen_net_codes(stm32ai_exe_path, model_path, opt.model_name, temp_path, output_model_path, version)

    gen_ai_model_codes(opt, utils_path, output_path)
    print('over common_deploy')

def deploy_main(opt, save_path, c_project_path, tflite_model_path):
    print(opt.__dict__)
    # export(opt, save_path)
    # tflite_path = os.path.join(save_path, "tflite")
    if c_project_path is None:
        c_project_path=save_path
    # deploy(opt, save_path, tflite_path, c_project_path)
    # with open(opt.config, 'r') as f:
    #     cfg = yaml.safe_load(f)
    cfg = {}
    # tflite_model_path = 'results/mobilenetv2_100.tflite'
    tfmodel = ClassifierTfOrt(cfg, tflite_model_path)
    gen_ai_model_codes = partial(gen_common_ic_codes, code_replace=code_replace, cfg=cfg, tfmodel=tfmodel)
    common_deploy(opt, save_path, tflite_model_path, c_project_path, gen_ai_model_codes)
    print('over')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    # parser.add_argument('--config', type=str, default='results/mobilenetv2_100.yaml',
    #                     help='Specify training profile *.data')
    # parser.add_argument('--model_path', type=str, default='results/mobilenetv2_100_float.tflite')
    # parser.add_argument('--model_path', type=str, default='results/simple-model/tfl20240807.tflite')
    parser.add_argument('--model_path', type=str, default='D:/data/02_CableTemperatureDetector/fromTang_20240827/best_model_noview_20240828.tflite')
    parser.add_argument('--model_name', type=str, default='network_1')
    # parser.add_argument('--convert_type', type=int, default=1,
    #                     help='only 1,for tflite')
    # parser.add_argument('--tflite_val_path', type=str, default='../../../datasets/food-101/validation',
    #                     help='The path where the image which quantity need is saved')
    parser.add_argument('--c_project_path', type=str, default="results/t4",
                        help='The path of c project,None= results/deploy/xxxx_00xx')
    parser.add_argument('--stm32cubeai_path', type=str,
                        default="C:/Program1/xcubeai/stedgeai-windows-9.0.0",
                        help='The path of stm32cubeai')
    parser.add_argument('--series', type=str, default="f4",
                        help='The series of gd32,f4 or h7')
    parser.add_argument('--eval', type=bool, default=False,
                        help='eval exported model')
    parser.add_argument('--compiler', type=int, default=1,
                        help='compiler type,0 for armcc,1 fro gcc')
    parser.add_argument('--img_size', type=int, nargs='+', default=None,
                        help='Specify the image size of the input for the exported model.the img size in config is default')
    parser.add_argument('--separation', type=int, default=0)
    parser.add_argument('--separation-scale', type=int, default=2)
    parser.add_argument('--class-name', type=str, default="no,fire")

    opt = parser.parse_args()
    save_path = opt.c_project_path
    c_project_path = opt.c_project_path
    tflite_model_path = opt.model_path
    deploy_main(opt, save_path, c_project_path, tflite_model_path)