/**
 * @file sensor_mq2.c
 * @brief MQ-2 传感器底层驱动实现（修正版）
 * 
 * 修正说明：
 * 1. ADC 参考电压改为 1.8V（与前端一致）
 * 2. 负载电阻 RL 改为 500Ω（与前端 0.5kΩ 一致）
 * 3. 传感器供电电压改为 5V（与前端一致）
 * 4. PPM 公式改为 pow(11.5428 * R0/Rs, 0.6549)（与前端一致）
 * 5. R0 根据实测 ADC=1200 校准为 0.4312 kΩ
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

/* ========== 硬件参数配置（与前端保持一致） ========== */
#define ADC_MAX_VALUE       4095.0f     // 12位 ADC 最大值
#define ADC_REF_VOLTAGE     1.8f        // ADC 参考电压 = 1.8V（前端使用 1.8V）
#define MQ2_LOAD_RES        0.5f        // 负载电阻 RL = 0.5kΩ = 500Ω（前端使用 0.5kΩ）
#define MQ2_SUPPLY_VOLTAGE  5.0f        // 传感器供电电压 = 5V（前端使用 5V）

/* PPM 拟合系数（与前端保持一致） */
#define MQ2_PPM_COEF_A      11.5428f    // ppm = pow(A * R0/Rs, B)
#define MQ2_PPM_COEF_B      0.6549f

/* R0 校准值 */
/* 根据实测：洁净空气 ADC ≈ 1200，计算得 R0 = 0.4312 kΩ */
/* 计算公式：V = 1200/4095*1.8 = 0.5275V, Rs = 0.5*(5/0.5275-1) = 4.239kΩ, R0 = Rs/9.83 = 0.4312kΩ */
#define MQ2_CALIBRATED_R0   0.4312f     // 校准后的 R0 值（单位：kΩ）

/* 滑动滤波窗口大小 */
#define FILTER_WINDOW_SIZE  5

/* ========== 全局变量 ========== */
static int g_filter_buf[FILTER_WINDOW_SIZE] = {0};
static int g_filter_index = 0;
static int g_filter_count = 0;

/* R0 值（运行时可更新） */
static float g_mq2_R0 = MQ2_CALIBRATED_R0;

/* ========== 内部函数 ========== */

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
 * @brief 将 ADC 原始值换算为烟雾浓度 (ppm)
 * 
 * 换算步骤（与前端公式完全一致）：
 * 1. ADC -> V_out: V_out = ADC/4095 * 1.8
 * 2. V_out -> Rs: Rs = 0.5 * (5/V_out - 1)  单位：kΩ
 * 3. Rs -> PPM: ppm = pow(11.5428 * R0/Rs, 0.6549)
 * 
 * @param filtered_adc 经滤波后的 ADC 原始值
 * @return 烟雾浓度 ppm
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
    /* 单位：kΩ（与前端一致） */
    if (v_out < 0.01f) {
        HILOG_WARN(LOG_CORE, "MQ-2 V_out too low: %.3fV", v_out);
        return 0.0f;
    }

    float rs = MQ2_LOAD_RES * (MQ2_SUPPLY_VOLTAGE / v_out - 1.0f);

    /* 保护：Rs 不应过小 */
    if (rs < 0.01f) {
        rs = 0.01f;
    }

    /* 3. 计算 PPM（与前端公式一致） */
    /* ppm = pow(11.5428 * R0 / Rs, 0.6549) */
    float ratio = g_mq2_R0 / rs;
    float ppm = powf(MQ2_PPM_COEF_A * ratio, MQ2_PPM_COEF_B);

    return ppm;
}

/* ========== 公开接口 ========== */

int get_mq2_smoke_ppm(int adc_channel)
{
    int raw = get_adc_value(adc_channel);
    if (raw < 0) {
        HILOG_ERROR(LOG_CORE, "get_adc_value failed with code %d", raw);
        return raw;
    }

    int filtered = filter_adc_value(raw);
    float ppm = adc_to_ppm(filtered);

    return (int)(ppm + 0.5f);
}

/**
 * @brief 设置 R0 校准值
 * @param r0 校准后的 R0 值（单位：kΩ）
 */
void mq2_set_r0(float r0)
{
    if (r0 > 0.0f) {
        g_mq2_R0 = r0;
        HILOG_INFO(LOG_CORE, "MQ-2 R0 updated: %.4f kΩ", r0);
    }
}

/**
 * @brief 获取当前 R0 值
 * @return 当前 R0 值（单位：kΩ）
 */
float mq2_get_r0(void)
{
    return g_mq2_R0;
}

/**
 * @brief 校准 MQ-2 传感器（在洁净空气中调用）
 * 
 * 使用步骤：
 * 1. 将传感器置于洁净空气中，预热至少 10 分钟
 * 2. 调用此函数，返回校准后的 R0 值
 * 3. 保存 R0 值供后续使用
 * 
 * @param adc_channel ADC 通道号
 * @return 成功返回 R0 值（kΩ），失败返回负值
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

    HILOG_INFO(LOG_CORE, "MQ-2 calibrated: ADC=%d, Vout=%.4fV, Rs=%.4fkΩ, R0=%.4fkΩ",
               filtered, v_out, rs, r0);

    return r0;
}

/**
 * @brief 重置滤波器
 */
void mq2_filter_reset(void)
{
    memset_s(g_filter_buf, sizeof(g_filter_buf), 0, sizeof(g_filter_buf));
    g_filter_index = 0;
    g_filter_count = 0;
}