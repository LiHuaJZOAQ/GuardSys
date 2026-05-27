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
    
    /**
     * 异步获取 MQ-2 烟雾传感器数据
     * @param adcChannel ADC通道号 (如 1 对应 in_voltage2_raw)
     * @returns Promise<{ smoke: number }> smoke 为ADC原始值或换算浓度
     */
    function getMq2Data(adcChannel: number): Promise<{ smoke: number }>;

    /**
     * 异步控制报警声光设备 (蜂鸣器与RGB LED)
     * @param status 报警等级 (0: 正常, 1: 警告, 2: 报警)
     * @param pins 执行器引脚分配映射
     * @param pins.buzzerPin 蜂鸣器引脚编号
     * @param pins.rPin RGB红灯引脚编号
     * @param pins.gPin RGB绿灯引脚编号
     * @param pins.bPin RGB蓝灯引脚编号
     * @returns Promise<void>
     */
    function setAlarmStatus(status: number, pins: { buzzerPin: number, rPin: number, gPin: number, bPin: number }): Promise<void>;

}
export default guardsys_napi;