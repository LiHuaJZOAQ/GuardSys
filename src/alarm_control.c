/**
 * @file alarm_control.c
 * @brief 报警声光控制底层实现
 * 直接使用 Linux sysfs 接口操作 GPIO。
 * 蜂鸣器与 RGB LED 均为高电平有效（共阴驱动），若为共阳需取反。
 */
#include "alarm_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/* 日志宏兼容处理：若编译环境无 hilog，将自动降级为 stderr 输出 */
#ifndef HILOG_ERROR
#define HILOG_ERROR(tag, fmt, ...) fprintf(stderr, "[ERROR][%s] " fmt "\n", tag, ##__VA_ARGS__)
#endif
#ifndef HILOG_INFO
#define HILOG_INFO(tag, fmt, ...)  fprintf(stderr, "[INFO][%s] " fmt "\n",  tag, ##__VA_ARGS__)
#endif
#define LOG_CORE "ALARM_CTRL"

/* Sysfs 路径定义 */
#define SYSFS_GPIO_EXPORT   "/sys/class/gpio/export"
#define SYSFS_GPIO_BASE     "/sys/class/gpio/gpio%d/"

/** @brief 检查 GPIO 是否已导出 */
static int gpio_is_exported(int pin) {
    char path[128];
    snprintf(path, sizeof(path), SYSFS_GPIO_BASE "value", pin);
    return (access(path, F_OK) == 0) ? 1 : 0;
}

/** @brief 导出 GPIO 节点 */
static int gpio_export(int pin) {
    FILE *fp = fopen(SYSFS_GPIO_EXPORT, "w");
    if (!fp) {
        HILOG_ERROR(LOG_CORE, "alarm: open export failed for gpio %d", pin);
        return -1;
    }
    fprintf(fp, "%d", pin);
    fclose(fp);
    return 0;
}

/** @brief 设置 GPIO 方向 (0:in, 1:out) */
static int gpio_set_direction(int pin, int dir) {
    char path[128];
    snprintf(path, sizeof(path), SYSFS_GPIO_BASE "direction", pin);
    FILE *fp = fopen(path, "r+");
    if (!fp) {
        HILOG_ERROR(LOG_CORE, "alarm: open direction failed for gpio %d", pin);
        return -2;
    }
    fprintf(fp, "%s", dir ? "out" : "in");
    fclose(fp);
    return 0;
}

/** @brief 设置 GPIO 电平 (0:low, 1:high) */
static int gpio_set_value(int pin, int val) {
    char path[128];
    snprintf(path, sizeof(path), SYSFS_GPIO_BASE "value", pin);
    FILE *fp = fopen(path, "r+");
    if (!fp) {
        HILOG_ERROR(LOG_CORE, "alarm: open value failed for gpio %d", pin);
        return -3;
    }
    fprintf(fp, "%d", val);
    fclose(fp);
    return 0;
}

/** @brief 内部函数：确保 GPIO 已导出并配置为输出 */
static int setup_gpio(int pin) {
    int ret;
    // 若未导出则执行导出（避免重复导出导致 EBUSY）
    if (!gpio_is_exported(pin)) {
        ret = gpio_export(pin);
        if (ret < 0) return ret;
        // sysfs 节点创建需要极短时间，加 10ms 延时防止后续读写竞争
        usleep(10000);
    }

    // 设置为输出方向
    ret = gpio_set_direction(pin, 1); // 1 表示 out
    if (ret < 0) return ret;

    return 0;
}

/**
 * @brief 根据报警状态设置 GPIO 组合电平
 */
int alarm_control_set(int status, int buzzerPin, int rPin, int gPin, int bPin) {
    int ret;
    int buzzerLevel = 0;
    int rLevel = 0, gLevel = 0, bLevel = 0;

    // 1. 确保所有 GPIO 已导出并设为输出
    ret = setup_gpio(buzzerPin); if (ret < 0) return ret;
    ret = setup_gpio(rPin);      if (ret < 0) return ret;
    ret = setup_gpio(gPin);      if (ret < 0) return ret;
    ret = setup_gpio(bPin);      if (ret < 0) return ret;

    // 2. 根据状态确定各引脚电平
    switch (status) {
        case 0: // 正常
            buzzerLevel = 1; 
            rLevel = 0; gLevel = 1; bLevel = 0;
            break;
        case 1: // 警告：蜂鸣器响 + 黄灯(R+G)
            buzzerLevel = 0;
            rLevel = 1; gLevel = 1; bLevel = 0;
            break;
        case 2: // 报警：蜂鸣器响 + 红灯
            buzzerLevel = 0;
            rLevel = 1; gLevel = 0; bLevel = 0;
            break;
        default:
            HILOG_ERROR(LOG_CORE, "alarm: invalid status %d", status);
            return -4; // 无效状态
    }

    // 3. 写入电平
    ret = gpio_set_value(buzzerPin, buzzerLevel); if (ret < 0) return ret;
    ret = gpio_set_value(rPin, rLevel);           if (ret < 0) return ret;
    ret = gpio_set_value(gPin, gLevel);           if (ret < 0) return ret;
    ret = gpio_set_value(bPin, bLevel);           if (ret < 0) return ret;

    HILOG_INFO(LOG_CORE, "alarm: set status %d, buzzer=%d, RGB=(%d,%d,%d)",
               status, buzzerLevel, rLevel, gLevel, bLevel);
    return 0;
}
