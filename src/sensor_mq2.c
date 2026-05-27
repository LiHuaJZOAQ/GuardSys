//to check

/**
 * @file sensor_mq2.c
 * @brief MQ-2 传感器底层驱动实现
 *
 * 该文件实现 ADC 原始值读取、滑动平均滤波以及基于分压公式的
 * 烟雾浓度 (ppm) 换算。采用 sysfs 接口访问 ADC IIO 设备。
 */

#include "sensor_mq2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <securec.h> // 提供 memset_s, strcpy_s 等安全函数
#include "hilog/log.h" // 假定日志组件，实际可按需替换

/* 定义 ADC sysfs 节点路径 */
#define ADC_CHANNEL_1 "/sys/bus/iio/devices/iio:device0/in_voltage2_raw"
#define ADC_CHANNEL_2 "/sys/bus/iio/devices/iio:device0/in_voltage3_raw"

/* 硬件参数配置 */
#define ADC_MAX_VALUE     4095.0f   // 12位 ADC 最大值
#define ADC_REF_VOLTAGE   3.3f      // ADC 参考电压 (V)
#define MQ2_LOAD_RES      10000.0f  // 负载电阻 RL = 10kΩ
#define MQ2_RO_CLEAN_AIR  9.83f     // 洁净空气中传感器电阻 Rs/R0 典型比值
                                     // 根据数据手册，MQ-2 在洁净空气中 Rs/R0 ≈ 9.83

/* 滑动滤波窗口大小 */
#define FILTER_WINDOW_SIZE 5

/* 滑动滤波缓冲区 (静态变量，保存最近 N 次 ADC 读数) */
static int g_filter_buf[FILTER_WINDOW_SIZE] = {0};
static int g_filter_index = 0;
static int g_filter_count = 0; // 记录已填充的样本数（用于开机后窗口未满时的平均）

/**
 * @brief 读取指定 ADC 通道的原始值
 *
 * 从 sysfs 节点读取字符串并转换为整数。
 *
 * @param adc_channel ADC 通道号 (1 或 2)
 * @return 成功返回 ADC 原始值 (0 ~ 4095)，失败返回负值错误码。
 */
static int get_adc_value(int adc_channel)
{
    char adc_path[128];
    memset_s(adc_path, sizeof(adc_path), 0, sizeof(adc_path));

    /* 根据通道号选择 sysfs 路径 */
    if (adc_channel == 1) {
        strcpy_s(adc_path, sizeof(adc_path), ADC_CHANNEL_1);
    } else if (adc_channel == 2) {
        strcpy_s(adc_path, sizeof(adc_path), ADC_CHANNEL_2);
    } else {
        HILOG_ERROR(LOG_CORE, "Invalid ADC channel: %d", adc_channel);
        return -1;
    }

    /* 打开 ADC 节点文件 */
    FILE *fp = fopen(adc_path, "r");
    if (fp == NULL) {
        HILOG_ERROR(LOG_CORE, "Failed to open ADC channel file: %s", adc_path);
        return -2;
    }

    /* 读取原始值字符串，缓冲区大小足够容纳最大字符数 (含 '\0') */
    char adc_value_str[16];
    memset_s(adc_value_str, sizeof(adc_value_str), 0, sizeof(adc_value_str));
    size_t read_bytes = fread(adc_value_str, sizeof(char), sizeof(adc_value_str) - 1, fp);
    fclose(fp);

    if (read_bytes <= 0) {
        HILOG_ERROR(LOG_CORE, "Failed to read ADC value from %s", adc_path);
        return -3;
    }

    /* 转换为整数值 */
    int adc_value = atoi(adc_value_str);
    return adc_value;
}

/**
 * @brief 滑动平均滤波
 *
 * 使用循环队列保存最近 FILTER_WINDOW_SIZE 次 ADC 值，
 * 返回当前滑动窗口内的平均值，实现软件去噪。
 *
 * @param raw_adc 最新一次的 ADC 原始值
 * @return 滤波后的 ADC 均值
 */
static int filter_adc_value(int raw_adc)
{
    /* 将新样本存入循环队列 */
    g_filter_buf[g_filter_index] = raw_adc;
    g_filter_index = (g_filter_index + 1) % FILTER_WINDOW_SIZE;

    /* 更新已填充的样本数，直到窗口满 */
    if (g_filter_count < FILTER_WINDOW_SIZE) {
        g_filter_count++;
    }

    /* 计算当前窗口内样本的平均值 */
    int sum = 0;
    for (int i = 0; i < g_filter_count; i++) {
        sum += g_filter_buf[i];
    }
    return sum / g_filter_count;
}

/**
 * @brief 将 ADC 原始值换算为烟雾浓度 (ppm)
 *
 * 换算步骤：
 * 1. ADC 值 -> 测量电压 V_out
 * 2. V_out -> 传感器电阻 Rs
 * 3. Rs/R0 比值 -> ppm（根据 MQ-2 灵敏度特性曲线拟合）
 *
 * 电路模型：V_out = Vref * (RL / (Rs + RL))
 * 推导得：Rs = RL * (Vref / V_out - 1)
 *
 * R0 为洁净空气中 Rs 的标定值，本例简化为通过 MQ2_RO_CLEAN_AIR 折算。
 * 实际产品需进行校准获得准确的 R0。
 *
 * 浓度换算公式取自 MQ-2 数据手册中的双对数曲线拟合：
 * ppm = a * (Rs/R0)^b
 * 选取两个已知点：(1, 10) 和 (9.83, 1) 近似解出：
 *   a ≈ 26.5，b ≈ -1.47
 * 不同气体参数不同，此处以可燃气体/烟雾近似。
 *
 * @param filtered_adc 经滤波后的 ADC 原始值
 * @return 烟雾浓度 ppm
 */
static float adc_to_ppm(int filtered_adc)
{
    /* 限制 ADC 范围，避免分母为零 */
    if (filtered_adc <= 0) {
        filtered_adc = 1;
    }
    if (filtered_adc >= ADC_MAX_VALUE) {
        filtered_adc = (int)ADC_MAX_VALUE - 1;
    }

    /* 1. 计算测量电压 */
    float v_out = ((float)filtered_adc / ADC_MAX_VALUE) * ADC_REF_VOLTAGE;

    /* 2. 计算传感器电阻 Rs */
    // 避免 v_out 接近 0 导致溢出
    float rs = MQ2_LOAD_RES * (ADC_REF_VOLTAGE / v_out - 1);

    /* 3. 计算 Rs/R0 比值 */
    // 这里 R0 取为 Rs 在洁净空气中的理论值 (通过比值折算)
    // 为简化，认为 R0 = Rs / MQ2_RO_CLEAN_AIR
    // 实际应当存储校准后的 R0
    float ratio = rs / MQ2_RO_CLEAN_AIR; // 近似 RS/R0

    /* 4. 幂函数拟合计算 ppm */
    // ppm = a * ratio^b
    // 拟合系数可调，此示例采用典型值
    const float a = 26.5f;
    const float b = -1.47f;
    float ppm = a * powf(ratio, b);

    return ppm;
}

/**
 * @brief 获取 MQ-2 烟雾浓度 (ppm)
 *
 * 整合 ADC 读取、滤波、换算流程。
 *
 * @param adc_channel ADC 通道号
 * @return >=0 表示浓度值，<0 表示错误码
 */
int get_mq2_smoke_ppm(int adc_channel)
{
    /* 1. 读取原始 ADC 值 */
    int raw = get_adc_value(adc_channel);
    if (raw < 0) {
        HILOG_ERROR(LOG_CORE, "get_adc_value failed with code %d", raw);
        return raw; // 透传错误码
    }

    /* 2. 滑动滤波 */
    int filtered = filter_adc_value(raw);

    /* 3. 换算为 ppm 并取整返回 (NAPI 端使用 number 类型) */
    float ppm = adc_to_ppm(filtered);
    return (int)(ppm + 0.5f); // 四舍五入
}
