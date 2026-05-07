/**
 * @file sensor_sht30.c
 * @brief SHT30 传感器 I2C 驱动实现
 */

#include "sensor_sht30.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/** SHT30 单次测量命令：高重复精度 (High Repeatability) */
#define SHT30_CMD_SINGLE_HIGH 0x2C06

/**
 * @brief 计算 SHT30 数据的 CRC-8 校验值
 * @details 使用 SHT3x 官方推荐的多项式：0x31 (x^8 + x^5 + x^4 + 1)
 *
 * @param data 指向待校验数据缓冲区的指针（通常包含 2 字节有效数据）
 * @param length 待校验数据的字节数
 * @return uint8_t 计算得到的 CRC-8 校验码
 */
static uint8_t sht30_calc_crc(const uint8_t *data, size_t length)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

int sht30_read_data(int i2c_bus, uint8_t address, Sht30DataResult *out_data)
{
    if (!out_data || i2c_bus < 0)
        return -1;

    char dev_path[32] = {0};
    snprintf(dev_path, sizeof(dev_path), "/dev/i2c-%d", i2c_bus);

    int fd = open(dev_path, O_RDWR);
    if (fd < 0)
        return -2;

    // 配置 I2C 超时与重试机制
    ioctl(fd, I2C_TIMEOUT, 1);
    ioctl(fd, I2C_RETRIES, 2);
    if (ioctl(fd, I2C_SLAVE, address) < 0)
    {
        close(fd);
        return -3;
    }

    // 构造单次读取命令
    uint8_t cmd[] = {(uint8_t)((SHT30_CMD_SINGLE_HIGH >> 8) & 0xFF),
                     (uint8_t)(SHT30_CMD_SINGLE_HIGH & 0xFF)};
    uint8_t read_buf[6] = {0}; // 2B Temp + 1B CRC + 2B Humi + 1B CRC

    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data i2c_data;

    msgs[0].addr = address;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = cmd;
    msgs[1].addr = address;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = 6;
    msgs[1].buf = read_buf;

    i2c_data.msgs = msgs;
    i2c_data.nmsgs = 2;

    // 执行 I2C 组合读写事务
    if (ioctl(fd, I2C_RDWR, &i2c_data) < 0)
    {
        close(fd);
        return -4;
    }

    // CRC 校验
    if (sht30_calc_crc(read_buf, 2) != read_buf[2] ||
        sht30_calc_crc(read_buf + 3, 2) != read_buf[5])
    {
        close(fd);
        return -5;
    }

    // 原始数据转换
    uint16_t raw_temp = ((uint16_t)read_buf[0] << 8) | read_buf[1];
    uint16_t raw_humi = ((uint16_t)read_buf[3] << 8) | read_buf[4];

    out_data->temp = 175.0f * (float)raw_temp / 65535.0f - 45.0f;
    out_data->humi = 100.0f * (float)raw_humi / 65535.0f;
    out_data->status = 0;

    close(fd);
    return 0;
}