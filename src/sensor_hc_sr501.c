/**
 * @file sensor_hc_sr501.c
 * @brief HC-SR501 红外传感器底层驱动实现
 * @details 基于 Sysfs GPIO 实现，包含引脚映射、导出、方向配置、读取及软件去抖
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "sensor_hc_sr501.h"
#include "hilog/log.h"

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "SR501_DRV"

// GPIO 路径宏定义
#define GPIO_EXPORT_PATH   "/sys/class/gpio/export"
#define GPIO_UNEXPORT_PATH "/sys/class/gpio/unexport"
#define GPIO_BASE_PATH     "/sys/class/gpio/gpio"

// UnionPi Tiger GPIO 映射: 逻辑引脚 1 -> GPIO380
#define GPIO_BASE_NUM 379

// 去抖参数
#define DEBOUNCE_COUNT 3
#define DEBOUNCE_DELAY_US 10000 // 10ms

/**
 * @brief 逻辑引脚转系统 GPIO 编号
 */
static int logic_to_sys_gpio(int logic_pin) {
    if (logic_pin < 1 || logic_pin > 16) {
        HILOG_ERROR(LOG_CORE, "Invalid logic pin: %{public}d", logic_pin);
        return -1;
    }
    return GPIO_BASE_NUM + logic_pin;
}

/**
 * @brief 检查 GPIO 是否已导出
 */
static int gpio_is_exported(int gpio_num) {
    char path[128];
    snprintf(path, sizeof(path), "%s%d/value", GPIO_BASE_PATH, gpio_num);
    return access(path, F_OK) == 0;
}

/**
 * @brief 导出 GPIO 引脚
 */
static int gpio_export(int gpio_num) {
    if (gpio_is_exported(gpio_num)) {
        return 0;
    }

    int fd = open(GPIO_EXPORT_PATH, O_WRONLY);
    if (fd < 0) {
        HILOG_ERROR(LOG_CORE, "Open export failed: %{public}s", strerror(errno));
        return -1;
    }

    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", gpio_num);
    if (write(fd, buf, len) < 0) {
        HILOG_ERROR(LOG_CORE, "Export gpio%{public}d failed: %{public}s", gpio_num, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    usleep(50000); // 等待 sysfs 节点创建完成
    return 0;
}

/**
 * @brief 设置 GPIO 方向为输入
 */
static int gpio_set_input(int gpio_num) {
    char path[128];
    snprintf(path, sizeof(path), "%s%d/direction", GPIO_BASE_PATH, gpio_num);

    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        HILOG_ERROR(LOG_CORE, "Open direction failed: %{public}s", strerror(errno));
        return -1;
    }

    if (write(fd, "in", 2) < 0) {
        HILOG_ERROR(LOG_CORE, "Set direction in failed: %{public}s", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/**
 * @brief 读取 GPIO 电平值 (0 或 1)
 */
static int gpio_read_value(int gpio_num) {
    char path[128];
    snprintf(path, sizeof(path), "%s%d/value", GPIO_BASE_PATH, gpio_num);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        HILOG_ERROR(LOG_CORE, "Open value failed: %{public}s", strerror(errno));
        return -1;
    }

    char buf[4] = {0};
    if (read(fd, buf, sizeof(buf) - 1) < 0) {
        HILOG_ERROR(LOG_CORE, "Read value failed: %{public}s", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return atoi(buf);
}

/**
 * @brief 获取红外状态 (含软件去抖)
 */
int sr501_get_status(int logic_pin, bool *status) {
    if (!status) {
        return -1;
    }

    int gpio_num = logic_to_sys_gpio(logic_pin);
    if (gpio_num < 0) {
        return -1;
    }

    // 1. 导出 GPIO
    if (gpio_export(gpio_num) != 0) {
        return -1;
    }

    // 2. 设置为输入
    if (gpio_set_input(gpio_num) != 0) {
        return -1;
    }

    // 3. 软件去抖：连续读取多次，结果一致才返回
    int high_count = 0;
    int low_count = 0;

    for (int i = 0; i < DEBOUNCE_COUNT; i++) {
        int val = gpio_read_value(gpio_num);
        if (val < 0) {
            return -1;
        }
        if (val == 1) {
            high_count++;
        } else {
            low_count++;
        }
        usleep(DEBOUNCE_DELAY_US);
    }

    // 多数表决
    *status = (high_count > low_count);
    HILOG_DEBUG(LOG_CORE, "SR501 Pin%{public}d IR Status: %{public}s", 
                logic_pin, *status ? "DETECTED" : "CLEAR");
    return 0;
}
