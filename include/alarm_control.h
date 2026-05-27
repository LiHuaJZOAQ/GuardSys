//to check

/**
 * @file alarm_control.h
 * @brief 报警声光控制底层驱动接口
 * 根据报警状态和引脚映射，控制蜂鸣器（有源）与 RGB LED。
 * GPIO 操作基于 sysfs 接口
 */
#ifndef ALARM_CONTROL_H
#define ALARM_CONTROL_H


#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 设置报警状态，控制蜂鸣器与 RGB LED
 * @param status     报警等级: 0 - 正常，1 - 警告，2 - 报警
 * @param buzzerPin  蜂鸣器 GPIO 编号
 * @param rPin       红色 LED GPIO 编号
 * @param gPin       绿色 LED GPIO 编号
 * @param bPin       蓝色 LED GPIO 编号
 * @return 0 成功；<0 失败，返回错误码
 *     -1: GPIO 导出失败
 *     -2: GPIO 方向设置失败
 *     -3: GPIO 电平设置失败
 *     -4: 无效的报警状态
 */
int alarm_control_set(int status, int buzzerPin, int rPin, int gPin, int bPin);

#ifdef __cplusplus
}
#endif

#endif // ALARM_CONTROL_H
