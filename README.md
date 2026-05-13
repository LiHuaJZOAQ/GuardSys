# 基于AI人脸识别的智能家居安防报警系统

## 南向项目目录结构

```text
guardsys/
├── BUILD.gn                  # 模块编译配置总文件
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
    ├── main.c                # 南向服务初始化与守护进程入口 (不含报警状态机) (quit temply)
    ├── sensor_hc_sr501.c     # 人体红外信号读取实现
    ├── sensor_mq2.c          # 烟雾ADC采集实现
    └── sensor_sht30.c        # 温湿度I2C读取实现
```

## 注意事项

项目开发中，目前项目中已有代码未经验证，谨慎参考