# 基于AI人脸识别的智能家居安防报警系统

## 南向项目目录结构

```text
sample/guardsys/
├── BUILD.gn                  # 模块编译配置总文件
├── include/                  # 接口定义头文件
│   ├── alarm_control.h       # 蜂鸣器与RGB LED控制头文件
│   ├── face_recognize.h      # OpenCV预处理与SeetaFace2识别头文件
│   ├── sensor_hc_sr501.h     # 人体红外传感器驱动头文件
│   ├── sensor_mq2.h          # 烟雾传感器驱动头文件
│   └── sensor_sht30.h        # 温湿度传感器驱动头文件
├── napi/                     # NAPI接口封装目录
│   ├── BUILD.gn              # NAPI动态库编译配置文件
│   └── guardsys_napi.cpp     # C++与ArkTS交互的接口实现文件
└── src/                      # 核心业务实现源码
    ├── alarm_control.c       # 报警声光执行逻辑实现
    ├── face_recognize.cpp    # 人脸采集、比对及去抖逻辑实现
    ├── main.c                # 南向服务初始化与守护进程入口 (不含报警状态机)
    ├── sensor_hc_sr501.c     # 人体红外信号读取实现
    ├── sensor_mq2.c          # 烟雾ADC采集实现
    └── sensor_sht30.c        # 温湿度I2C读取实现
```

## 注意事项

目前项目中已有代码未经验证，谨慎参考