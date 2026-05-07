/**
 * @file sensor_sht30.h
 * @brief SHT30 温湿度传感器底层驱动接口声明
 * @details 提供标准的 C 语言 API，供 NAPI 层或其他业务模块调用。
 */

#ifndef SENSOR_SHT30_H
#define SENSOR_SHT30_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @struct Sht30DataResult
     * @brief SHT30 读取结果数据结构
     */
    typedef struct
    {
        float temp; /**< 温度值，单位：摄氏度(℃) */
        float humi; /**< 相对湿度值，单位：百分比(%) */
        int status; /**< 状态码：0 表示成功；负数表示具体错误原因 */
    } Sht30DataResult;

    /**
     * @brief 从指定 I2C 总线读取 SHT30 传感器的实时温湿度数据
     * @details 采用单次触发模式（高重复精度），自动处理 CRC-8 校验与原始数据换算。
     *          该函数为阻塞型调用，建议在异步工作线程中使用以避免阻塞 UI 主线程。
     *
     * @param i2c_bus I2C 总线号（例如：4 对应 Linux 设备节点 /dev/i2c-4）
     * @param address SHT30 的 I2C 从机地址（常见为 0x44 或 0x45）
     * @param out_data [out] 输出参数，用于接收读取到的温湿度数据及执行状态
     * @return int
     *         - 0: 读取成功
     *         - -1: 参数非法或内存分配失败
     *         - -2: 打开 /dev/i2c-x 设备节点失败
     *         - -3: ioctl 设置从机地址失败
     *         - -4: I2C 读写传输失败
     *         - -5: CRC 校验失败，数据丢弃
     */
    int sht30_read_data(int i2c_bus, uint8_t address, Sht30DataResult *out_data);

#ifdef __cplusplus
}
#endif

#endif // SENSOR_SHT30_H