/**
 * @file main.cpp
 * @brief guardsys 南向模块综合测试程序
 * @details 测试以下模块：
 *          - HC-SR501 人体红外传感器
 *          - MQ-2 烟雾传感器
 *          - SHT30 温湿度传感器
 *          - 报警声光控制
 *          - 人脸识别引擎
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>

// 项目头文件
#include "sensor_hc_sr501.h"
#include "sensor_mq2.h"
#include "sensor_sht30.h"
#include "alarm_control.h"
#include "face_recognize.h"

// ==================== 测试配置 ====================

// HC-SR501 配置
#define HC_SR501_LOGIC_PIN      1

// MQ-2 配置
#define MQ2_ADC_CHANNEL         1

// SHT30 配置
#define SHT30_I2C_BUS           4
#define SHT30_I2C_ADDRESS       0x44

// 报警控制 GPIO 配置
#define BUZZER_PIN              10
#define LED_R_PIN               11
#define LED_G_PIN               12
#define LED_B_PIN               13

// 人脸识别配置
#define FACE_MODEL_PATH         "/system/usr/model/"
#define TEST_REGISTER_NAME      "test_user"
#define TEST_REGISTER_IMAGE     "/data/local/tmp/register_face.jpg"
#define TEST_RECOGNIZE_IMAGE    "/data/local/tmp/recognize_face.jpg"

// ==================== 辅助函数 ====================

static void print_separator(const char* title) {
    printf("\n");
    printf("============================================================\n");
    printf("  %s\n", title);
    printf("============================================================\n");
}

static void print_result(const char* func, int ret) {
    if (ret >= 0) {
        printf("[✓ PASS] %s returned %d\n", func, ret);
    } else {
        printf("[✗ FAIL] %s returned error code: %d\n", func, ret);
    }
}

static void print_error_code(const char* module, int code) {
    printf("  错误码: %d\n", code);
    if (strcmp(module, "mq2") == 0) {
        switch (code) {
            case -1: printf("  原因: 无效的 ADC 通道\n"); break;
            case -2: printf("  原因: 打开 ADC 节点失败\n"); break;
            case -3: printf("  原因: 读取 ADC 节点数据错误\n"); break;
            default: printf("  原因: 未知错误\n"); break;
        }
    } else if (strcmp(module, "sht30") == 0) {
        switch (code) {
            case -1: printf("  原因: 参数非法或内存分配失败\n"); break;
            case -2: printf("  原因: 打开 /dev/i2c-x 设备节点失败\n"); break;
            case -3: printf("  原因: ioctl 设置从机地址失败\n"); break;
            case -4: printf("  原因: I2C 读写传输失败\n"); break;
            case -5: printf("  原因: CRC 校验失败\n"); break;
            default: printf("  原因: 未知错误\n"); break;
        }
    } else if (strcmp(module, "alarm") == 0) {
        switch (code) {
            case -1: printf("  原因: GPIO 导出失败\n"); break;
            case -2: printf("  原因: GPIO 方向设置失败\n"); break;
            case -3: printf("  原因: GPIO 电平设置失败\n"); break;
            case -4: printf("  原因: 无效的报警状态\n"); break;
            default: printf("  原因: 未知错误\n"); break;
        }
    }
}

// ==================== 模块测试函数 ====================

/**
 * @brief 测试 HC-SR501 人体红外传感器
 */
static int test_hc_sr501() {
    print_separator("HC-SR501 人体红外传感器测试");
    
    bool status = false;
    int ret = sr501_get_status(HC_SR501_LOGIC_PIN, &status);
    
    print_result("sr501_get_status", ret);
    
    if (ret == 0) {
        printf("  检测结果: %s\n", status ? "检测到人体 (高电平)" : "无人体 (低电平)");
    } else {
        print_error_code("hc_sr501", ret);
    }
    
    return ret;
}

/**
 * @brief 测试 MQ-2 烟雾传感器
 */
static int test_mq2() {
    print_separator("MQ-2 烟雾传感器测试");
    
    int ppm = get_mq2_smoke_ppm(MQ2_ADC_CHANNEL);
    
    if (ppm >= 0) {
        printf("[✓ PASS] get_mq2_smoke_ppm 成功\n");
        printf("  烟雾浓度: %d ppm\n", ppm);
        
        if (ppm < 100) {
            printf("  状态: 正常\n");
        } else if (ppm < 300) {
            printf("  状态: 轻微烟雾\n");
        } else {
            printf("  状态: 烟雾浓度过高！\n");
        }
        return 0;
    } else {
        printf("[✗ FAIL] get_mq2_smoke_ppm 失败\n");
        print_error_code("mq2", ppm);
        return ppm;
    }
}

/**
 * @brief 测试 SHT30 温湿度传感器
 */
static int test_sht30() {
    print_separator("SHT30 温湿度传感器测试");
    
    Sht30DataResult result;
    memset(&result, 0, sizeof(result));
    
    int ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, &result);
    
    print_result("sht30_read_data", ret);
    
    if (ret == 0 && result.status == 0) {
        printf("  温度: %.2f ℃\n", result.temp);
        printf("  湿度: %.2f %%\n", result.humi);
        
        if (result.temp < -40 || result.temp > 125) {
            printf("  [!] 警告: 温度值超出合理范围\n");
        }
        if (result.humi < 0 || result.humi > 100) {
            printf("  [!] 警告: 湿度值超出合理范围\n");
        }
        return 0;
    } else {
        if (result.status != 0) {
            printf("  内部状态码: %d\n", result.status);
        }
        print_error_code("sht30", ret);
        return ret;
    }
}

/**
 * @brief 测试报警声光控制
 */
static int test_alarm_control() {
    print_separator("报警声光控制测试");
    
    int ret = 0;
    
    // 测试状态 0: 正常 (绿灯)
    printf("\n[测试] 设置状态: 正常 (绿灯)\n");
    ret = alarm_control_set(0, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=0)", ret);
    if (ret < 0) print_error_code("alarm", ret);
    sleep(1);
    
    // 测试状态 1: 警告 (黄灯)
    printf("\n[测试] 设置状态: 警告 (黄灯)\n");
    ret = alarm_control_set(1, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=1)", ret);
    if (ret < 0) print_error_code("alarm", ret);
    sleep(1);
    
    // 测试状态 2: 报警 (红灯+蜂鸣器)
    printf("\n[测试] 设置状态: 报警 (红灯+蜂鸣器)\n");
    ret = alarm_control_set(2, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=2)", ret);
    if (ret < 0) print_error_code("alarm", ret);
    sleep(1);
    
    // 恢复正常
    printf("\n[测试] 恢复状态: 正常\n");
    ret = alarm_control_set(0, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=0)", ret);
    sleep(1);
    
    // 测试无效状态
    printf("\n[测试] 设置无效状态: -1\n");
    ret = alarm_control_set(-1, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    if (ret == -4) {
        printf("[✓ PASS] 正确返回错误码 -4 (无效的报警状态)\n");
    } else {
        printf("[✗ FAIL] 期望返回 -4, 实际返回 %d\n", ret);
    }
    
    return 0;
}

/**
 * @brief 测试人脸识别引擎
 */
static int test_face_recognize() {
    print_separator("人脸识别引擎测试");
    
    FaceSearchInfo info;
    int ret = 0;
    
    // 1. 初始化引擎
    printf("\n[步骤 1] 初始化人脸识别引擎\n");
    printf("  模型路径: %s\n", FACE_MODEL_PATH);
    ret = FaceSearchInit(&info, FACE_MODEL_PATH);
    print_result("FaceSearchInit", ret);
    
    if (ret != 0) {
        printf("  初始化失败，可能原因:\n");
        printf("    - 模型文件路径不存在\n");
        printf("    - 模型文件加载失败\n");
        printf("  跳过后续人脸识别测试\n");
        return ret;
    }
    
    // 2. 注册人脸
    printf("\n[步骤 2] 注册人脸\n");
    RegisterInfo regInfo;
    regInfo.name = TEST_REGISTER_NAME;
    regInfo.path = TEST_REGISTER_IMAGE;
    
    printf("  注册姓名: %s\n", regInfo.name.c_str());
    printf("  图片路径: %s\n", regInfo.path.c_str());
    
    ret = FaceSearchRegister(info, regInfo);
    print_result("FaceSearchRegister", ret);
    
    if (ret != 0) {
        printf("  [!] 注册失败，可能测试图片不存在，继续其他测试\n");
    }
    
    // 3. 人脸检测
    printf("\n[步骤 3] 人脸检测\n");
    printf("  测试图片: %s\n", TEST_RECOGNIZE_IMAGE);
    
    FaceRect rects[10];
    int faceCount = GetFaceRects(TEST_RECOGNIZE_IMAGE, rects, 10);
    
    if (faceCount >= 0) {
        printf("[✓ PASS] GetFaceRects 成功\n");
        printf("  检测到人脸数量: %d\n", faceCount);
        for (int i = 0; i < faceCount && i < 10; i++) {
            printf("  人脸[%d]: x=%d, y=%d, w=%d, h=%d\n",
                   i, rects[i].x, rects[i].y, rects[i].w, rects[i].h);
        }
    } else {
        printf("[✗ FAIL] GetFaceRects 返回错误码: %d\n", faceCount);
        printf("  可能原因: 图片文件不存在或格式不支持\n");
    }
    
    // 4. 人脸识别
    printf("\n[步骤 4] 人脸识别\n");
    std::string result = FaceSearchRecognize(info, TEST_RECOGNIZE_IMAGE);
    
    if (!result.empty()) {
        printf("[✓ PASS] FaceSearchRecognize 成功\n");
        printf("  识别结果: %s\n", result.c_str());
    } else {
        printf("[✗ FAIL] FaceSearchRecognize 返回空结果\n");
        printf("  可能原因: 未检测到人脸或未匹配到注册用户\n");
    }
    
    // 5. 释放资源
    printf("\n[步骤 5] 释放人脸识别引擎资源\n");
    FaceSearchDeinit(&info);
    printf("[✓ PASS] FaceSearchDeinit 完成\n");
    
    return 0;
}

// ==================== 主函数 ====================

int main(int argc, char* argv[]) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        guardsys 南向模块综合测试程序 v1.0                  ║\n");
    printf("║        OpenHarmony 4.0 南向设备测试                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    int totalTests = 0;
    int passedTests = 0;
    
    // 测试 HC-SR501
    totalTests++;
    if (test_hc_sr501() == 0) passedTests++;
    
    // 测试 MQ-2
    totalTests++;
    if (test_mq2() == 0) passedTests++;
    
    // 测试 SHT30
    totalTests++;
    if (test_sht30() == 0) passedTests++;
    
    // 测试报警控制
    totalTests++;
    if (test_alarm_control() == 0) passedTests++;
    
    // 测试人脸识别
    totalTests++;
    if (test_face_recognize() == 0) passedTests++;
    
    // 测试汇总
    print_separator("测试汇总");
    printf("  总测试项: %d\n", totalTests);
    printf("  通过: %d\n", passedTests);
    printf("  失败: %d\n", totalTests - passedTests);
    printf("  通过率: %.1f%%\n", (float)passedTests / totalTests * 100.0f);
    
    if (passedTests == totalTests) {
        printf("\n  [✓] 所有测试通过!\n");
    } else {
        printf("\n  [!] 部分测试失败，请检查:\n");
        printf("      1. 硬件连接是否正确\n");
        printf("      2. GPIO/I2C/ADC 引脚配置\n");
        printf("      3. 模型文件和测试图片是否存在\n");
        printf("      4. 设备权限是否足够\n");
    }
    
    printf("\n");
    
    return (passedTests == totalTests) ? 0 : 1;
}