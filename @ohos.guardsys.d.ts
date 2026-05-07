declare namespace guardsys {
    /**
     * 异步获取 SHT30 温湿度
     * @param i2cBus I2C总线号 (如 4)
     * @param address 设备地址 (如 0x44)
     * @returns Promise<{ temp: number, humi: number }>
     */
    function getSht30Data(i2cBus: number, address: number): Promise<{ temp: number, humi: number }>;
}
export default guardsys;