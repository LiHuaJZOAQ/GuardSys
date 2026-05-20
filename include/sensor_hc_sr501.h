/**
 * @file sensor_hc_sr501.h
 * @brief HC-SR501 红外传感器底层驱动接口声明
 */
#ifndef SENSOR_HC_SR501_H
#define SENSOR_HC_SR501_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取 HC-SR501 红外状态
 * @param logic_pin 逻辑引脚编号 (如 1)
 * @param status 输出参数，true 表示检测到人体(高电平)，false 表示无人体
 * @return int 0 成功，<0 失败
 */
int sr501_get_status(int logic_pin, bool *status);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_HC_SR501_H