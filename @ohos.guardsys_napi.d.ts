declare namespace guardsys_napi {
//    前端有问题时加入下面文档注释
//    * @since 9
//    * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
    
    //new face modules , to check , without any verification

    
    /**
     * 人脸矩形框结构
     */
    interface FaceRect {
        x: number;
        y: number;
        w: number;
        h: number;
    }

    /**
     * 人脸注册参数
     */
    interface FaceRegisterParams {
        name: string;
        images: string[];
    }

    /**
     * 初始化人脸识别引擎
     * @returns Promise<number> 0 成功，<0 失败
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function faceInit(): Promise<number>;

    /**
     * 释放人脸识别引擎资源
     * @returns Promise<number> 0 成功
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function faceDeinit(): Promise<number>;

    /**
     * 获取图片中的人脸框坐标
     * @param imagePath 图片路径
     * @returns Promise<FaceRect[]> 人脸框数组
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function getFaceRects(imagePath: string): Promise<FaceRect[]>;

    /**
     * 注册人脸到识别引擎
     * @param params 注册参数 { name, images }
     * @returns Promise<number> 成功注册的图片数量
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function faceRegister(params: FaceRegisterParams): Promise<number>;

    /**
     * 识别图片中的人脸
     * @param imagePath 图片路径
     * @returns Promise<string> 识别结果（姓名或错误信息）
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function faceRecognize(imagePath: string): Promise<string>;
    //new face modules end


    /**
     * 异步获取 SHT30 温湿度
     * @param i2cBus I2C总线号 (如 4)
     * @param address 设备地址 (如 0x44)
     * @returns Promise<{ temp: number, humi: number }>
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function getSht30Data(i2cBus: number, address: number): Promise<{ temp: number, humi: number }>;
    
    /**
     * 异步获取 HC-SR501 红外人体感应状态
     * @param gpioPin 逻辑引脚编号 (如 1，底层自动映射为系统 GPIO 编号)
     * @returns Promise<{ ir: boolean }> ir 为 true 表示检测到人体
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function getIrStatus(gpioPin: number): Promise<{ ir: boolean }>;
    
    /**
     * 异步获取 MQ-2 烟雾传感器数据
     * @param adcChannel ADC通道号 (如 1 对应 in_voltage2_raw)
     * @returns Promise<{ smoke: number }> smoke 为ADC原始值或换算浓度
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function getMq2Data(adcChannel: number): Promise<{ smoke: number }>;

    /**
     * 异步控制报警声光设备 (蜂鸣器与RGB LED)
     * @param status 报警等级 (0: 正常, 1: 警告, 2: 报警)
     * @param pins 执行器引脚分配映射
     * @param pins.buzzerPin 蜂鸣器引脚编号(物理引脚，如380)
     * @param pins.rPin RGB红灯引脚编号(物理引脚，如380)
     * @param pins.gPin RGB绿灯引脚编号(物理引脚，如380)
     * @param pins.bPin RGB蓝灯引脚编号(物理引脚，如380)
     * @returns Promise<void>
     * @since 9
     * @syscap SystemCapability.Ability.AbilityRuntime.AbilityCore
     */
    function setAlarmStatus(status: number, pins: { buzzerPin: number, rPin: number, gPin: number, bPin: number }): Promise<void>;

}
export default guardsys_napi;