# 基于AI人脸识别的智能家居安防报警系统

## 南向项目目录结构

```text
guardsys/
├── @ohos.guardsys.d.ts       # OHOS NAPI TypeScript 声明文件
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

## 编译构建

1.修改`bundle.json`中相关路径和子系统名称，然后在`vendor/unionman/unionpi_tiger/config.json`中添加组件。
```json
{
    "component": "guardsys",
    "features": [
    
    ]
}
```

2.运行编译构建指令


```bash
./build.sh --product-name unionpi_tiger --ccache --build-target guardsys_napi
```

部分编译构建可以减少编译时间，防止ninja file重新编译(该方法仅适用于不是新增部件的情况，如果第一次编译不可使用该方法。注意把`/home/user/OpenHarmony4.0Release/`改为实际路径)
```bash
/home/user/OpenHarmony4.0Release/prebuilts/build-tools/linux-x86/bin/ninja -w dupbuild=warn -C /home/user/OpenHarmony4.0Release/out/unionpi_tiger guardsys_napi
```

或者全量编译
```bash
./build.sh --product-name unionpi_tiger --ccache
```

3.编译完成后可以找到编译产物`out/unionpi_tiger/sample/guardsys/libguardsys_napi.z.so`

## 注意事项

项目开发中，目前项目中已有代码未经验证，谨慎参考
