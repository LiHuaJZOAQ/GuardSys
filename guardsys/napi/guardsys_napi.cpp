#include "napi/native_api.h"
#include <string>
#include <string.h>

// --- 以下为假设引入的南向 C 函数声明 ---
extern "C" int SHT30_Config(int bus_num, int i2c_addr);
extern "C" int SHT30_ReadData(float *temp, float *hum);
extern "C" int MQ2_Config(int device_num, int channel);
extern "C" int MQ2_ReadSmoke(float *concentration);
extern "C" int HC_SR501_Config(const char* gpio_pin);
extern "C" int HC_SR501_ReadStatus(int *isTriggered);
extern "C" void Alarm_Config(const char* buzzer, const char* r, const char* g, const char* b);
extern "C" int Alarm_SetStatus(int status);
// ----------------------------------------

// NAPI Function: configSensorInterface
static napi_value ConfigSensorInterface(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    char sensorType[32] = {0};
    size_t typeLen;
    napi_get_value_string_utf8(env, args[0], sensorType, sizeof(sensorType), &typeLen);

    // 解析针对不同传感器的配置并调用底层的 Config 函数
    if (strcmp(sensorType, "sht30") == 0) {
        napi_value busVal, addrVal;
        int busNum = 4, i2cAddr = 0x44;
        if (napi_get_named_property(env, args[1], "i2cBus", &busVal) == napi_ok) napi_get_value_int32(env, busVal, &busNum);
        if (napi_get_named_property(env, args[1], "address", &addrVal) == napi_ok) napi_get_value_int32(env, addrVal, &i2cAddr);
        SHT30_Config(busNum, i2cAddr);
    } else if (strcmp(sensorType, "mq2") == 0) {
        napi_value adcVal;
        int adcChannel = 1; // 默认传数值 1 代表 ADC1
        if (napi_get_named_property(env, args[1], "adcChannel", &adcVal) == napi_ok) {
            napi_get_value_int32(env, adcVal, &adcChannel);
        }
        
        // 1 代表 ADC1 (对应 channel=2), 2 代表 ADC2 (对应 channel=3)
        int deviceNum = 0;
        int channel = (adcChannel == 2) ? 3 : 2; 
        MQ2_Config(deviceNum, channel);

    } else if (strcmp(sensorType, "hc_sr501") == 0) {
        napi_value pinVal;
        int gpioPin = 1; // 默认传数值 1 代表 GPIO_1
        if (napi_get_named_property(env, args[1], "gpioPin", &pinVal) == napi_ok) {
            napi_get_value_int32(env, pinVal, &gpioPin);
        }
        
        // 映射数值 1~16 到系统编号 380~395
        int gpioNum = 380;
        if (gpioPin >= 1 && gpioPin <= 16) {
            gpioNum = 380 + (gpioPin - 1);
        }
        char sysPin[16];
        sprintf(sysPin, "%d", gpioNum);
        HC_SR501_Config(sysPin);

    } else if (strcmp(sensorType, "alarm") == 0) {
        napi_value buzzerVal, rVal, gVal, bVal;
        int buzzerPin = 2, rPin = 3, gPin = 4, bPin = 5;
        
        if (napi_get_named_property(env, args[1], "buzzerPin", &buzzerVal) == napi_ok) napi_get_value_int32(env, buzzerVal, &buzzerPin);
        if (napi_get_named_property(env, args[1], "rPin", &rVal) == napi_ok) napi_get_value_int32(env, rVal, &rPin);
        if (napi_get_named_property(env, args[1], "gPin", &gVal) == napi_ok) napi_get_value_int32(env, gVal, &gPin);
        if (napi_get_named_property(env, args[1], "bPin", &bVal) == napi_ok) napi_get_value_int32(env, bVal, &bPin);
        
        // 解析 1~16 为底层编号
        auto parseGpio = [](int pin) -> int {
            if (pin >= 1 && pin <= 16) return 380 + (pin - 1);
            return 380; // fallback
        };
        
        char sysBuzzer[16], sysR[16], sysG[16], sysB[16];
        sprintf(sysBuzzer, "%d", parseGpio(buzzerPin));
        sprintf(sysR, "%d", parseGpio(rPin));
        sprintf(sysG, "%d", parseGpio(gPin));
        sprintf(sysB, "%d", parseGpio(bPin));
        
        Alarm_Config(sysBuzzer, sysR, sysG, sysB);
    }

    napi_value ret;
    napi_get_boolean(env, true, &ret);
    return ret;
}

// NAPI Function: getSht30Data
static napi_value GetSht30Data(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_create_object(env, &result);
    
    float temp = 0.0, humi = 0.0;
    if (SHT30_ReadData(&temp, &humi) == 0) {
        napi_value tVal, hVal;
        napi_create_double(env, temp, &tVal);
        napi_create_double(env, humi, &hVal);
        napi_set_named_property(env, result, "temp", tVal);
        napi_set_named_property(env, result, "humi", hVal);
    }
    return result;
}

// NAPI Function: getMq2Data
static napi_value GetMq2Data(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_create_object(env, &result);
    
    float smoke = 0.0;
    if (MQ2_ReadSmoke(&smoke) == 0) {
        napi_value sVal;
        napi_create_double(env, smoke, &sVal);
        napi_set_named_property(env, result, "smoke", sVal);
    }
    return result;
}

// NAPI Function: getIrStatus
static napi_value GetIrStatus(napi_env env, napi_callback_info info) {
    napi_value result;
    napi_create_object(env, &result);
    
    int ir = 0;
    if (HC_SR501_ReadStatus(&ir) == 0) {
        napi_value irVal;
        napi_get_boolean(env, ir == 1, &irVal);
        napi_set_named_property(env, result, "ir", irVal);
    }
    return result;
}

// NAPI Function: setAlarmStatus
static napi_value SetAlarmStatus(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    int status = 0;
    napi_get_value_int32(env, args[0], &status);
    
    // Call underlying Alarm_SetStatus(status);
    
    napi_value ret;
    napi_get_undefined(env, &ret);
    return ret;
}

// Init Function
EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"configSensorInterface", nullptr, ConfigSensorInterface, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getSht30Data", nullptr, GetSht30Data, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getMq2Data", nullptr, GetMq2Data, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getIrStatus", nullptr, GetIrStatus, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setAlarmStatus", nullptr, SetAlarmStatus, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

// Module definition
static napi_module guardsysModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "guardsys",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterGuardsysModule(void) {
    napi_module_register(&guardsysModule);
}
