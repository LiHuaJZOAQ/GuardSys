/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#ifdef __cplusplus
#include "i2cinterface.h"
extern "C" {
#endif

// 全局变量声明

static sRHAndTemp_t tempRH;

static const uint16_t device_addr = 0x45;

static float TemperatureC;
static float Humidity;
static float TemperatureF;

char dev_name[] = "/dev/i2c-5";

// 同步方法实现软重启
static napi_value SoftReset(napi_env env, napi_callback_info info)
{
    int value;
    napi_value ret = nullptr;
    value = SoftReset(dev_name, device_addr);
    usleep(50L * 1000L);
    NAPI_CALL(env, napi_create_int32(env, value, &ret));
    return ret;
}

// 同步方法实现模式选择
static napi_value ModeSet(napi_env env, napi_callback_info info)
{
    // 获取JS传入的参数
    size_t argc = 2L;
    napi_value args[2L] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    // 验证参数数据类型
    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype0 == napi_number && valuetype1 == napi_number, "Wrong argument type. Number expected.");

    // NAPI类型数据转为C/C++类型
    int mps;
    NAPI_CALL(env, napi_get_value_int32(env, args[0], &mps));
    int repeatability;
    NAPI_CALL(env, napi_get_value_int32(env, args[1], &repeatability));

    // 业务执行
    int value;
    napi_value ret = nullptr;
    value = ModeSet(dev_name, device_addr, mps, repeatability);
    usleep(20L * 1000L);
    NAPI_CALL(env, napi_create_int32(env, value, &ret));
    return ret;
}

// 同步方法实现获取读取数据
static napi_value ReadData(napi_env env, napi_callback_info info)
{
    int value;
    napi_value ret = nullptr;
    value = ReadTempAndHum(dev_name, device_addr, &tempRH);
    usleep(20L * 1000L);
    NAPI_CALL(env, napi_create_double(env, value, &ret));
    TemperatureC = tempRH.TemperatureC;
    Humidity = tempRH.Humidity;
    TemperatureF = tempRH.TemperatureF;
    return ret;
}

// 将摄氏温度传到APP(运行此函数前需要运行ReadData函数)
static napi_value ReadTemperatureC(napi_env env, napi_callback_info info)
{
    napi_value tempC = nullptr;
    NAPI_CALL(env, napi_create_double(env, TemperatureC, &tempC));
    return tempC;
}
// 将湿度传到APP(运行此函数前需要运行ReadData函数)
static napi_value ReadHumidity(napi_env env, napi_callback_info info)
{
    napi_value hum = nullptr;
    NAPI_CALL(env, napi_create_double(env, Humidity, &hum));
    return hum;
}
// 将华氏温度传到APP(运行此函数前需要运行ReadData函数)
static napi_value ReadTemperatureF(napi_env env, napi_callback_info info)
{
    napi_value tempF = nullptr;
    NAPI_CALL(env, napi_create_double(env, TemperatureF, &tempF));
    return tempF;
}

// 注册接口
static napi_value registeri2cNapiDemoApis(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("SoftReset", SoftReset),
        DECLARE_NAPI_FUNCTION("ModeSet", ModeSet),
        DECLARE_NAPI_FUNCTION("ReadData", ReadData),
        DECLARE_NAPI_FUNCTION("ReadTemperatureC", ReadTemperatureC),
        DECLARE_NAPI_FUNCTION("ReadHumidity", ReadHumidity),
        DECLARE_NAPI_FUNCTION("ReadTemperatureF", ReadTemperatureF),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

// 模块定义
static napi_module i2cnapidemoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = registeri2cNapiDemoApis,
    .nm_modname = "i2cnapidemo",
    .nm_priv = ((void *)0),
    .reserved = {0},
};
#ifdef __cplusplus
}
#endif
// 注册模块
extern "C" __attribute__((constructor)) void Registeri2cNapidemoModule(void)
{
    napi_module_register(&i2cnapidemoModule); // 接口注册函数
}
