# 基于AI人脸识别的智能家居安防报警系统（南向）

## 项目简介

本项目基于OpenHarmony 4.0 Release版本，使用C/C++语言开发，实现了智能家居安防报警南向系统。系统包括人体红外传感器、烟雾传感器、温湿度传感器、蜂鸣器、RGB LED灯等硬件设备，实现了对应的南向驱动程序，并实现NAPI接口使得北向应用可以通过NAPI接口与硬件设备进行交互，实现了人体探测、烟雾检测、环境温湿度数据采集、危险报警等功能。

## 技术栈

C/C++ | OpenHarmony 4.0 Release | NAPI (Native API) | Ninja

## 开发环境

### 软件环境
- Ubuntu 20.04
- OpenHarmony 4.0 Release源码

### 硬件环境
- Union-Pi Tiger开发板(A311D)
- HC-SR501人体红外传感器(GPIO协议)
- MQ-2烟雾传感器(ADC协议)
- SHT30温湿度传感器(I2C协议)
- 蜂鸣器(GPIO协议)
- RGB LED灯(GPIO协议)
- 摄像头

## 南向项目目录结构

```text
guardsys/
├── @ohos.guardsys.d.ts       # OHOS NAPI TypeScript 声明文件
├── BUILD.gn                  # 模块编译配置总文件
├── bundle.json               # 模块配置文件
├── migrate/                  # 第三方库移植相关文件目录
├── models/                   # 模型文件目录
├── test/                     # 测试程序目录
├── include/                  # 接口定义头文件
│   ├── alarm_control.h       # 蜂鸣器与RGB LED控制头文件
│   ├── face_recognize.h      # OpenCV预处理与SeetaFace2识别头文件
│   ├── sensor_hc_sr501.h     # 人体红外传感器驱动头文件
│   ├── sensor_mq2.h          # 烟雾传感器驱动头文件
│   └── sensor_sht30.h        # 温湿度传感器驱动头文件
├── napi/                     # NAPI接口封装目录
│   ├── BUILD.gn              # 动态库编译配置（聚合所有 *_napi.cpp）
│   ├── guardsys_napi.h       # [公共] 异步基类、辅助函数、各模块描述符外部声明
│   ├── guardsys_napi.cpp     # [中枢] 模块注册中枢：聚合描述符并导出单一 SO 库
│   ├── sht30_napi.cpp        # [模块A] SHT30 温湿度异步接口实现
│   ├── hc_sr501_napi.cpp     # [模块B] HC-SR501 人体红外异步接口实现
│   ├── mq2_napi.cpp          # [模块C] MQ-2 烟雾传感器异步接口实现
│   ├── alarm_napi.cpp        # [模块D] 报警执行器（蜂鸣器/LED）同步/异步接口
│   └── face_recognize_napi.cpp # [模块E] 人脸识别 NPU 推理异步接口
└── src/                      # 核心业务实现源码
    ├── alarm_control.c       # 报警声光执行逻辑实现
    ├── face_recognize.cpp    # 人脸采集、比对及去抖逻辑实现
    ├── sensor_hc_sr501.c     # 人体红外信号读取实现
    ├── sensor_mq2.c          # 烟雾ADC采集实现
    └── sensor_sht30.c        # 温湿度I2C读取实现
```

## 编译构建

1. 修改`bundle.json`中相关路径和子系统名称，然后在`vendor/unionman/unionpi_tiger/config.json`中添加组件。
```json
{
    "component": "guardsys",
    "features": [
    
    ]
}
```

2. 运行编译构建指令


```bash
./build.sh --product-name unionpi_tiger --ccache --build-target guardsys_napi
```

直接使用ninja构建可以减少编译时间，防止ninja file重新编译(该方法仅适用于目标不是新增部件的情况，如果第一次编译不可使用该方法。注意把`/home/user/OpenHarmony4.0Release/`改为实际路径)
```bash
/home/user/OpenHarmony4.0Release/prebuilts/build-tools/linux-x86/bin/ninja -w dupbuild=warn -C /home/user/OpenHarmony4.0Release/out/unionpi_tiger guardsys_napi
```

或者全量编译
```bash
./build.sh --product-name unionpi_tiger --ccache
```

3. 编译完成后可以找到编译产物`out/unionpi_tiger/sample/guardsys/libguardsys_napi.z.so`，可以将其推送至开发板。
```bash
hdc shell mount -o remount,rw /
hdc file send libguardsys_napi.z.so /system/lib/module
```
注意需要同步推送opencv和seetaface的库文件，否则会报错，任何一个NAPI都无法使用。

4. 配置开发板的相关权限，可以参考`util/init.A311D.cfg`的配置。

```bash
hdc shell mount -o remount,rw /vendor
hdc file recv /vendor/etc/init.A311D.cfg ./
hdc file send init.A311D.cfg /vendor/etc
hdc shell reboot
```

## 项目进度

> [!WARNING]
> 目前项目中人脸识别模块因硬件问题无法验证，代码仅供参考

### 基本传感器的NAPI

- [x] 【NAPI-1/4】完成SHT30的NAPI编写（未验证）
- [x]  SHT30的NAPI验证通过
- [x] 【NAPI-2/4】完成HC-SR501的NAPI编写（未验证）
- [ ]  HC-SR501的NAPI验证通过
- [x] 【NAPI-3/4】完成MQ-2的NAPI编写（未验证）
- [x]  MQ-2的NAPI验证通过
- [x] 【NAPI-4/4】完成蜂鸣器与RGB LED的NAPI编写（未验证）
- [ ]  蜂鸣器的NAPI验证通过
- [x]  RGB LED的NAPI验证通过
- [x]  基本传感器的NAPI编译通过
- [ ]  基本传感器的NAPI验证通过

### 人脸识别模块

- [x]  OpenCV库移植
- [x]  Seetaface库移植
- [x]  人脸识别模块NAPI编写
- [x]  人脸识别模块编译通过

## 许可证

本项目遵循[GNU Affero General Public License v3.0](LICENSE)许可证。

## 贡献

如果有任何建议或想要贡献代码，请随时提交Pull Request或创建Issue。