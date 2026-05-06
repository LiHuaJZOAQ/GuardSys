#include <stdio.h>
#include <unistd.h>

extern "C" int SHT30_Config(int bus_num, int i2c_addr);
extern "C" int MQ2_Config(int device_num, int channel);
extern "C" int HC_SR501_Config(const char* gpio_pin);
extern "C" void Alarm_Config(const char* buzzer, const char* r, const char* g, const char* b);

// 南向守护进程/测试主函数 (不包含状态机逻辑，只负责硬件初始化校验)
int main(int argc, char **argv) {
    printf("GuardSys Southbound Daemon Start...\n");
    
    // 底层可做一些默认初始化，但实际参数由 NAPI 在应用层下发
    // UnionPi Tiger ADC1 (device0, channel2), HC-SR501 (GPIO 380), Alarm (GPIO 381~384)
    SHT30_Config(4, 0x44);
    MQ2_Config(0, 2);
    HC_SR501_Config("380");
    Alarm_Config("381", "382", "383", "384");
    
    printf("Hardware nodes configured via defaults. Waiting for NAPI overrides...\n");
    
    // 保持进程存活 (如果作为独立服务运行)
    while (1) {
        sleep(10);
    }
    
    return 0;
}
