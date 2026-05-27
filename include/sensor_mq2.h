//to check

/**
 * @file sensor_mq2.h
 * @brief MQ-2 烟雾传感器底层驱动接口
 *
 * 提供 ADC 读取、滑动滤波及电压/浓度换算功能。
 * 适用于 IIO 子系统，通过 sysfs 节点读取原始 ADC 值。
 */

#ifndef SENSOR_MQ2_H
#define SENSOR_MQ2_H

#include <stdint.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取 MQ-2 传感器的烟雾浓度 (ppm)
 *
 * 通过指定的 ADC 通道读取原始值，经滑动滤波与电压换算后得到烟雾浓度。
 *
 * @param adc_channel ADC 通道号，1 或 2。
 * @return 成功时返回浓度值 (ppm)，失败时返回负值错误码。
 *         - -1: 无效的 ADC 通道
 *         - -2: 打开 ADC 节点失败
 *         - -3: 读取 ADC 节点数据错误
 */
int get_mq2_smoke_ppm(int adc_channel);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_MQ2_H
