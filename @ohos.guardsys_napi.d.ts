declare namespace guardsys_napi {
//    前端有问题时加入下面文档注释
//    * @since 9
//    * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
    /**
     * 异步获取 SHT30 温湿度
     * @param i2cBus I2C总线号 (如 4)
     * @param address 设备地址 (如 0x44)
     * @returns Promise<{ temp: number, humi: number }>
     */
    function getSht30Data(i2cBus: number, address: number): Promise<{ temp: number, humi: number }>;
    /**
     * 异步获取 HC-SR501 红外人体感应状态
     * @param gpioPin 逻辑引脚编号 (如 1，底层自动映射为系统 GPIO 编号)
     * @returns Promise<{ ir: boolean }> ir 为 true 表示检测到人体
     */
    function getIrStatus(gpioPin: number): Promise<{ ir: boolean }>;
}
export default guardsys_napi;