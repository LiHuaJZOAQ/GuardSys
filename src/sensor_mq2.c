/**
 * @file sensor_mq2.c
 * @brief MQ-2 传感器底层驱动实现（修正版）
 * 
 * 修正说明：
 * 1. 根据前端逻辑，传感器供电电压为 5V，负载电阻 RL = 500Ω
 * 2. ADC 参考电压使用 3.3V（九联平台标准）
 * 3. PPM 公式与前端保持一致：ppm = pow(11.5428 * R0/Rs, 0.6549)
 * 4. 添加 R0 校准机制，R0 需在实际洁净空气中校准获得
 */

#include "sensor_mq2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <securec.h>
#include "hilog/log.h"

/* 定义 ADC sysfs 节点路径 */
#define ADC_CHANNEL_1 "/sys/bus/iio/devices/iio:device0/in_voltage2_raw"
#define ADC_CHANNEL_2 "/sys/bus/iio/devices/iio:device0/in_voltage3_raw"

/* ========== 硬件参数配置 ========== */
/* 注意：根据前端代码推导的实际参数 */
#define ADC_MAX_VALUE       4095.0f     // 12位 ADC 最大值
#define ADC_REF_VOLTAGE     3.3f        // ADC 参考电压 (九联平台为 3.3V)
#define MQ2_LOAD_RES        500.0f      // 负载电阻 RL = 500Ω (前端使用 0.5kΩ)
#define MQ2_SUPPLY_VOLTAGE  5.0f        // 传感器供电电压 = 5V (前端使用 5V)

/* PPM 拟合系数（与前端保持一致） */
#define MQ2_PPM_COEF_A      11.5428f    // ppm = pow(A * R0/Rs, B)
#define MQ2_PPM_COEF_B      0.6549f

/* 默认 R0 值（建议通过校准获得实际值） */
#define MQ2_DEFAULT_R0      6.64f       // 前端使用的校准值

/* 滑动滤波窗口大小 */
#define FILTER_WINDOW_SIZE  5

/* ========== 全局变量 ========== */
/* 滑动滤波缓冲区 */
static int g_filter_buf[FILTER_WINDOW_SIZE] = {0};
static int g_filter_index = 0;
static int g_filter_count = 0;

/* R0 校准值（运行时可更新） */
static float g_mq2_R0 = MQ2_DEFAULT_R0;

/* ========== 内部函数 ========== */

/**
 * @brief 读取指定 ADC 通道的原始值
 */
static int get_adc_value(int adc_channel)
{
    char adc_path[128];
    memset_s(adc_path, sizeof(adc_path), 0, sizeof(adc_path));

    if (adc_channel == 1) {
        strcpy_s(adc_path, sizeof(adc_path), ADC_CHANNEL_1);
    } else if (adc_channel == 2) {
        strcpy_s(adc_path, sizeof(adc_path), ADC_CHANNEL_2);
    } else {
        HILOG_ERROR(LOG_CORE, "Invalid ADC channel: %d", adc_channel);
        return -1;
    }

    FILE *fp = fopen(adc_path, "r");
    if (fp == NULL) {
        HILOG_ERROR(LOG_CORE, "Failed to open ADC channel file: %s", adc_path);
        return -2;
    }

    char adc_value_str[16];
    memset_s(adc_value_str, sizeof(adc_value_str), 0, sizeof(adc_value_str));
    size_t read_bytes = fread(adc_value_str, sizeof(char), sizeof(adc_value_str) - 1, fp);
    fclose(fp);

    if (read_bytes <= 0) {
        HILOG_ERROR(LOG_CORE, "Failed to read ADC value from %s", adc_path);
        return -3;
    }

    return atoi(adc_value_str);
}

/**
 * @brief 滑动平均滤波
 */
static int filter_adc_value(int raw_adc)
{
    g_filter_buf[g_filter_index] = raw_adc;
    g_filter_index = (g_filter_index + 1) % FILTER_WINDOW_SIZE;

    if (g_filter_count < FILTER_WINDOW_SIZE) {
        g_filter_count++;
    }

    int sum = 0;
    for (int i = 0; i < g_filter_count; i++) {
        sum += g_filter_buf[i];
    }
    return sum / g_filter_count;
}

/**
 * @brief 将 ADC 值换算为 PPM（修正版，与前端公式一致）
 * 
 * 换算流程：
 * 1. ADC -> V_out: V_out = ADC/4095 * 3.3V
 * 2. V_out -> Rs: Rs = RL * (V_supply/V_out - 1) = 500 * (5.0/V_out - 1)
 * 3. Rs -> PPM: ppm = pow(11.5428 * R0/Rs, 0.6549)
 */
static float adc_to_ppm(int filtered_adc)
{
    /* 限制 ADC 范围 */
    if (filtered_adc <= 0) {
        filtered_adc = 1;
    }
    if (filtered_adc >= (int)ADC_MAX_VALUE) {
        filtered_adc = (int)ADC_MAX_VALUE - 1;
    }

    /* 1. 计算测量电压 V_out */
    float v_out = ((float)filtered_adc / ADC_MAX_VALUE) * ADC_REF_VOLTAGE;

    /* 2. 计算传感器电阻 Rs */
    /* Rs = RL * (V_supply / V_out - 1) */
    /* 保护：避免 v_out 过小导致溢出 */
    if (v_out < 0.01f) {
        HILOG_WARN(LOG_CORE, "MQ-2 V_out too low: %.3fV, sensor may be shorted", v_out);
        return 0.0f;
    }

    float rs = MQ2_LOAD_RES * (MQ2_SUPPLY_VOLTAGE / v_out - 1.0f);

    /* 保护：Rs 不应为负或过小 */
    if (rs < 1.0f) {
        rs = 1.0f;
    }

    /* 3. 计算 PPM */
    /* ppm = pow(A * R0 / Rs, B) */
    float ratio = g_mq2_R0 / rs;
    float ppm = powf(MQ2_PPM_COEF_A * ratio, MQ2_PPM_COEF_B);

    return ppm;
}

/* ========== 公开接口 ========== */

/**
 * @brief 获取 MQ-2 烟雾浓度 (ppm)
 */
int get_mq2_smoke_ppm(int adc_channel)
{
    int raw = get_adc_value(adc_channel);
    if (raw < 0) {
        HILOG_ERROR(LOG_CORE, "get_adc_value failed with code %d", raw);
        return raw;
    }

    int filtered = filter_adc_value(raw);
    float ppm = adc_to_ppm(filtered);

    return (int)(ppm + 0.5f);  // 四舍五入
}

/**
 * @brief 设置 R0 校准值
 * @param r0 校准后的 R0 值（单位：kΩ）
 */
void mq2_set_r0(float r0)
{
    if (r0 > 0.0f) {
        g_mq2_R0 = r0;
        HILOG_INFO(LOG_CORE, "MQ-2 R0 updated: %.2f", r0);
    }
}

/**
 * @brief 获取当前 R0 值
 */
float mq2_get_r0(void)
{
    return g_mq2_R0;
}

/**
 * @brief 校准 MQ-2 传感器（在洁净空气中调用）
 * 
 * 校准流程：
 * 1. 将传感器置于洁净空气中稳定 5-10 分钟
 * 2. 调用此函数获取当前 Rs 值
 * 3. R0 = Rs / 9.83（根据数据手册，洁净空气中 Rs/R0 ≈ 9.83）
 * 4. 保存 R0 值供后续使用
 * 
 * @param adc_channel ADC 通道号
 * @return 成功返回校准后的 R0 值，失败返回负值
 */
float mq2_calibrate(int adc_channel)
{
    int raw = get_adc_value(adc_channel);
    if (raw < 0) {
        HILOG_ERROR(LOG_CORE, "Calibration failed: get_adc_value error %d", raw);
        return (float)raw;
    }

    int filtered = filter_adc_value(raw);

    /* 计算 V_out */
    float v_out = ((float)filtered / ADC_MAX_VALUE) * ADC_REF_VOLTAGE;
    if (v_out < 0.01f) {
        HILOG_ERROR(LOG_CORE, "Calibration failed: V_out too low %.3fV", v_out);
        return -4.0f;
    }

    /* 计算 Rs */
    float rs = MQ2_LOAD_RES * (MQ2_SUPPLY_VOLTAGE / v_out - 1.0f);

    /* 计算 R0 = Rs / 9.83 */
    float r0 = rs / 9.83f;

    /* 更新 R0 */
    g_mq2_R0 = r0;

    HILOG_INFO(LOG_CORE, "MQ-2 calibrated: Rs=%.2fΩ, R0=%.2fΩ", rs, r0);
    return r0;
}

/**
 * @brief 重置滤波器（用于重新初始化）
 */
void mq2_filter_reset(void)
{
    memset_s(g_filter_buf, sizeof(g_filter_buf), 0, sizeof(g_filter_buf));
    g_filter_index = 0;
    g_filter_count = 0;
}