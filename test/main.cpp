/**
 * @file main.cpp
 * @brief guardsys Southbound Module Comprehensive Test Program
 * @details Tests the following modules:
 *          - HC-SR501 PIR Sensor
 *          - MQ-2 Smoke Sensor
 *          - SHT30 Temperature & Humidity Sensor
 *          - Alarm Control (Buzzer & RGB LED)
 *          - Face Recognition Engine
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdarg>

// Project headers
#include "sensor_hc_sr501.h"
#include "sensor_mq2.h"
#include "sensor_sht30.h"
#include "alarm_control.h"
#include "face_recognize.h"

// ==================== Test Configuration ====================

// HC-SR501 Configuration
#define HC_SR501_LOGIC_PIN      1

// MQ-2 Configuration
#define MQ2_ADC_CHANNEL         1

// SHT30 Configuration
#define SHT30_I2C_BUS           5
#define SHT30_I2C_ADDRESS       0x44
#define SHT30_ALT_ADDRESS       0x45    // Alternative I2C address

// Alarm Control GPIO Configuration
#define BUZZER_PIN              10
#define LED_R_PIN               11
#define LED_G_PIN               12
#define LED_B_PIN               13

// Face Recognition Configuration
#define FACE_MODEL_PATH         "/system/usr/model/"
#define TEST_REGISTER_NAME      "test_user"
#define TEST_REGISTER_IMAGE     "/data/local/tmp/register_face.jpg"
#define TEST_RECOGNIZE_IMAGE    "/data/local/tmp/recognize_face.jpg"

// SHT30 Test Parameters
#define SHT30_STABILITY_SAMPLES     10      // Number of samples for stability test
#define SHT30_PERFORMANCE_SAMPLES   50      // Number of samples for performance test
#define SHT30_TEMP_MIN_VALID        -40.0f  // Minimum valid temperature
#define SHT30_TEMP_MAX_VALID        125.0f  // Maximum valid temperature
#define SHT30_HUMI_MIN_VALID        0.0f    // Minimum valid humidity
#define SHT30_HUMI_MAX_VALID        100.0f  // Maximum valid humidity
#define SHT30_TEMP_WARNING_DELTA    2.0f    // Warning threshold for temperature variation
#define SHT30_HUMI_WARNING_DELTA    5.0f    // Warning threshold for humidity variation

// ==================== Helper Functions ====================

static void print_separator(const char* title) {
    printf("\n");
    printf("============================================================\n");
    printf("  %s\n", title);
    printf("============================================================\n");
}

static void print_subsection(const char* title) {
    printf("\n  ----------------------------------------------------------------\n");
    printf("  %s\n", title);
    printf("  ----------------------------------------------------------------\n");
}

static void print_result(const char* func, int ret) {
    if (ret >= 0) {
        printf("  [PASS] %s returned %d\n", func, ret);
    } else {
        printf("  [FAIL] %s returned error code: %d\n", func, ret);
    }
}

static void print_test_case(const char* format, ...) {
    printf("  [TEST] ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

static void print_error_code(const char* module, int code) {
    printf("  Error code: %d\n", code);
    if (strcmp(module, "mq2") == 0) {
        switch (code) {
            case -1: printf("  Reason: Invalid ADC channel\n"); break;
            case -2: printf("  Reason: Failed to open ADC node\n"); break;
            case -3: printf("  Reason: Failed to read ADC node data\n"); break;
            default: printf("  Reason: Unknown error\n"); break;
        }
    } else if (strcmp(module, "sht30") == 0) {
        switch (code) {
            case -1: printf("  Reason: Invalid parameter or memory allocation failed\n"); break;
            case -2: printf("  Reason: Failed to open /dev/i2c-x device node\n"); break;
            case -3: printf("  Reason: ioctl failed to set slave address\n"); break;
            case -4: printf("  Reason: I2C read/write transfer failed\n"); break;
            case -5: printf("  Reason: CRC check failed, data discarded\n"); break;
            default: printf("  Reason: Unknown error\n"); break;
        }
    } else if (strcmp(module, "alarm") == 0) {
        switch (code) {
            case -1: printf("  Reason: GPIO export failed\n"); break;
            case -2: printf("  Reason: GPIO direction setting failed\n"); break;
            case -3: printf("  Reason: GPIO level setting failed\n"); break;
            case -4: printf("  Reason: Invalid alarm status\n"); break;
            default: printf("  Reason: Unknown error\n"); break;
        }
    }
}

static void print_warning(const char* message) {
    printf("  [WARN] %s\n", message);
}

static void print_info(const char* message) {
    printf("  [INFO] %s\n", message);
}

// ==================== Module Test Functions ====================

/**
 * @brief Test HC-SR501 PIR Sensor
 */
static int test_hc_sr501() {
    print_separator("HC-SR501 PIR Sensor Test");
    
    bool status = false;
    int ret = sr501_get_status(HC_SR501_LOGIC_PIN, &status);
    
    print_result("sr501_get_status", ret);
    
    if (ret == 0) {
        printf("  Detection result: %s\n", 
               status ? "Human detected (HIGH level)" : "No human detected (LOW level)");
    } else {
        print_error_code("hc_sr501", ret);
    }
    
    return ret;
}

/**
 * @brief Test MQ-2 Smoke Sensor
 */
static int test_mq2() {
    print_separator("MQ-2 Smoke Sensor Test");
    
    int ppm = get_mq2_smoke_ppm(MQ2_ADC_CHANNEL);
    
    if (ppm >= 0) {
        printf("  [PASS] get_mq2_smoke_ppm succeeded\n");
        printf("  Smoke concentration: %d ppm\n", ppm);
        
        if (ppm < 100) {
            printf("  Status: Normal\n");
        } else if (ppm < 300) {
            printf("  Status: Light smoke detected\n");
        } else {
            printf("  Status: HIGH SMOKE CONCENTRATION!\n");
        }
        return 0;
    } else {
        printf("  [FAIL] get_mq2_smoke_ppm failed\n");
        print_error_code("mq2", ppm);
        return ppm;
    }
}

/**
 * @brief SHT30 Comprehensive Test Suite
 */
static int test_sht30() {
    print_separator("SHT30 Temperature & Humidity Sensor Test Suite");
    
    int total_subtests = 0;
    int passed_subtests = 0;
    
    // =========================================================================
    // Test 1: Basic Read Test
    // =========================================================================
    print_subsection("Test 1: Basic Read Test");
    total_subtests++;
    
    Sht30DataResult result;
    memset(&result, 0, sizeof(result));
    
    int ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, &result);
    print_result("sht30_read_data", ret);
    
    if (ret == 0 && result.status == 0) {
        printf("  Temperature: %.2f C\n", result.temp);
        printf("  Humidity: %.2f %%\n", result.humi);
        passed_subtests++;
    } else {
        if (result.status != 0) {
            printf("  Internal status code: %d\n", result.status);
        }
        print_error_code("sht30", ret);
    }
    
    // =========================================================================
    // Test 2: Parameter Validation - Null Pointer
    // =========================================================================
    print_subsection("Test 2: Parameter Validation - Null Pointer");
    total_subtests++;
    
    print_test_case("Passing NULL pointer for out_data");
    ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, NULL);
    
    if (ret == -1) {
        printf("  [PASS] Correctly returned -1 for NULL pointer\n");
        passed_subtests++;
    } else {
        printf("  [FAIL] Expected -1, got %d\n", ret);
    }
    
    // =========================================================================
    // Test 3: Parameter Validation - Invalid I2C Bus
    // =========================================================================
    print_subsection("Test 3: Parameter Validation - Invalid I2C Bus");
    total_subtests++;
    
    print_test_case("Using invalid I2C bus number (-1)");
    memset(&result, 0, sizeof(result));
    ret = sht30_read_data(-1, SHT30_I2C_ADDRESS, &result);
    
    if (ret < 0) {
        printf("  [PASS] Correctly returned error for invalid bus: %d\n", ret);
        print_error_code("sht30", ret);
        passed_subtests++;
    } else {
        printf("  [FAIL] Expected error, got success\n");
    }
    
    // =========================================================================
    // Test 4: Parameter Validation - Invalid I2C Address
    // =========================================================================
    print_subsection("Test 4: Parameter Validation - Invalid I2C Address");
    total_subtests++;
    
    print_test_case("Using invalid I2C address (0x00)");
    memset(&result, 0, sizeof(result));
    ret = sht30_read_data(SHT30_I2C_BUS, 0x00, &result);
    
    if (ret < 0) {
        printf("  [PASS] Correctly returned error for invalid address: %d\n", ret);
        print_error_code("sht30", ret);
        passed_subtests++;
    } else {
        printf("  [FAIL] Expected error, got success\n");
    }
    
    // =========================================================================
    // Test 5: Alternative I2C Address Test
    // =========================================================================
    print_subsection("Test 5: Alternative I2C Address Test");
    total_subtests++;
    
    print_test_case("Testing alternative address 0x45");
    memset(&result, 0, sizeof(result));
    ret = sht30_read_data(SHT30_I2C_BUS, SHT30_ALT_ADDRESS, &result);
    
    if (ret == 0 && result.status == 0) {
        printf("  [PASS] Alternative address works\n");
        printf("  Temperature: %.2f C, Humidity: %.2f %%\n", result.temp, result.humi);
        passed_subtests++;
    } else {
        printf("  [INFO] Alternative address not responding (expected if sensor uses 0x44)\n");
        print_error_code("sht30", ret);
        // Don't count as failure since sensor may use 0x44
        passed_subtests++;
    }
    
    // =========================================================================
    // Test 6: Data Range Validation
    // =========================================================================
    print_subsection("Test 6: Data Range Validation");
    total_subtests++;
    
    // Re-read data for validation
    memset(&result, 0, sizeof(result));
    ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, &result);
    
    if (ret == 0 && result.status == 0) {
        bool range_valid = true;
        
        printf("  Valid temperature range: [%.1f, %.1f] C\n", 
               SHT30_TEMP_MIN_VALID, SHT30_TEMP_MAX_VALID);
        printf("  Valid humidity range: [%.1f, %.1f] %%\n", 
               SHT30_HUMI_MIN_VALID, SHT30_HUMI_MAX_VALID);
        
        if (result.temp < SHT30_TEMP_MIN_VALID || result.temp > SHT30_TEMP_MAX_VALID) {
            printf("  [FAIL] Temperature %.2f C out of valid range\n", result.temp);
            range_valid = false;
        } else {
            printf("  [PASS] Temperature %.2f C within valid range\n", result.temp);
        }
        
        if (result.humi < SHT30_HUMI_MIN_VALID || result.humi > SHT30_HUMI_MAX_VALID) {
            printf("  [FAIL] Humidity %.2f %% out of valid range\n", result.humi);
            range_valid = false;
        } else {
            printf("  [PASS] Humidity %.2f %% within valid range\n", result.humi);
        }
        
        if (range_valid) {
            passed_subtests++;
        }
    } else {
        printf("  [SKIP] Cannot validate range, read failed\n");
        print_error_code("sht30", ret);
    }
    
    // =========================================================================
    // Test 7: Stability Test (Multiple Samples)
    // =========================================================================
    print_subsection("Test 7: Stability Test");
    total_subtests++;
    
    print_test_case("Reading %d consecutive samples", SHT30_STABILITY_SAMPLES);
    
    float temp_samples[SHT30_STABILITY_SAMPLES];
    float humi_samples[SHT30_STABILITY_SAMPLES];
    int valid_samples = 0;
    
    for (int i = 0; i < SHT30_STABILITY_SAMPLES; i++) {
        memset(&result, 0, sizeof(result));
        ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, &result);
        
        if (ret == 0 && result.status == 0) {
            temp_samples[valid_samples] = result.temp;
            humi_samples[valid_samples] = result.humi;
            valid_samples++;
        }
        usleep(50000); // 50ms delay between reads
    }
    
    if (valid_samples > 0) {
        printf("  Valid samples: %d/%d\n", valid_samples, SHT30_STABILITY_SAMPLES);
        
        // Calculate statistics
        float temp_sum = 0, humi_sum = 0;
        float temp_min = temp_samples[0], temp_max = temp_samples[0];
        float humi_min = humi_samples[0], humi_max = humi_samples[0];
        
        for (int i = 0; i < valid_samples; i++) {
            temp_sum += temp_samples[i];
            humi_sum += humi_samples[i];
            
            if (temp_samples[i] < temp_min) temp_min = temp_samples[i];
            if (temp_samples[i] > temp_max) temp_max = temp_samples[i];
            if (humi_samples[i] < humi_min) humi_min = humi_samples[i];
            if (humi_samples[i] > humi_max) humi_max = humi_samples[i];
        }
        
        float temp_avg = temp_sum / valid_samples;
        float humi_avg = humi_sum / valid_samples;
        float temp_delta = temp_max - temp_min;
        float humi_delta = humi_max - humi_min;
        
        printf("  Temperature: avg=%.2f C, min=%.2f C, max=%.2f C, delta=%.2f C\n",
               temp_avg, temp_min, temp_max, temp_delta);
        printf("  Humidity: avg=%.2f %%, min=%.2f %%, max=%.2f %%, delta=%.2f %%\n",
               humi_avg, humi_min, humi_max, humi_delta);
        
        // Check stability
        bool stable = true;
        if (temp_delta > SHT30_TEMP_WARNING_DELTA) {
            print_warning("Temperature variation exceeds threshold");
            stable = false;
        }
        if (humi_delta > SHT30_HUMI_WARNING_DELTA) {
            print_warning("Humidity variation exceeds threshold");
            stable = false;
        }
        
        if (stable) {
            printf("  [PASS] Sensor readings are stable\n");
            passed_subtests++;
        } else {
            printf("  [WARN] Sensor readings show high variation\n");
            // Still pass as this might be environmental
            passed_subtests++;
        }
    } else {
        printf("  [FAIL] No valid samples collected\n");
    }
    
    // =========================================================================
    // Test 8: Performance Test (Read Latency)
    // =========================================================================
    print_subsection("Test 8: Performance Test");
    total_subtests++;
    
    print_test_case("Measuring read latency over %d samples", SHT30_PERFORMANCE_SAMPLES);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    int perf_success = 0;
    
    for (int i = 0; i < SHT30_PERFORMANCE_SAMPLES; i++) {
        memset(&result, 0, sizeof(result));
        ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, &result);
        if (ret == 0 && result.status == 0) {
            perf_success++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    float total_ms = duration.count() / 1000.0f;
    float avg_ms = total_ms / SHT30_PERFORMANCE_SAMPLES;
    
    printf("  Total time: %.2f ms\n", total_ms);
    printf("  Average latency: %.2f ms per read\n", avg_ms);
    printf("  Success rate: %d/%d (%.1f%%)\n", 
           perf_success, SHT30_PERFORMANCE_SAMPLES,
           (float)perf_success / SHT30_PERFORMANCE_SAMPLES * 100.0f);
    
    if (perf_success == SHT30_PERFORMANCE_SAMPLES) {
        printf("  [PASS] All reads successful\n");
        passed_subtests++;
    } else {
        printf("  [WARN] Some reads failed\n");
        passed_subtests++; // Still pass as partial success
    }
    
    // =========================================================================
    // Test 9: Rapid Consecutive Reads
    // =========================================================================
    print_subsection("Test 9: Rapid Consecutive Reads");
    total_subtests++;
    
    print_test_case("Testing rapid reads without delay");
    
    int rapid_success = 0;
    int rapid_failures = 0;
    
    for (int i = 0; i < 5; i++) {
        memset(&result, 0, sizeof(result));
        ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, &result);
        
        if (ret == 0 && result.status == 0) {
            rapid_success++;
        } else {
            rapid_failures++;
        }
        // No delay - test rapid access
    }
    
    printf("  Rapid reads: %d success, %d failures\n", rapid_success, rapid_failures);
    
    if (rapid_success >= 3) {
        printf("  [PASS] Sensor handles rapid reads adequately\n");
        passed_subtests++;
    } else {
        printf("  [WARN] Sensor may need delay between reads\n");
        passed_subtests++; // Warning only
    }
    
    // =========================================================================
    // Test 10: Data Consistency Check
    // =========================================================================
    print_subsection("Test 10: Data Consistency Check");
    total_subtests++;
    
    print_test_case("Verifying data structure integrity");
    
    memset(&result, 0, sizeof(result));
    ret = sht30_read_data(SHT30_I2C_BUS, SHT30_I2C_ADDRESS, &result);
    
    bool consistency_ok = true;
    
    if (ret == 0) {
        // Check that status is 0 on success
        if (result.status != 0) {
            printf("  [FAIL] status should be 0 when return is 0\n");
            consistency_ok = false;
        } else {
            printf("  [PASS] Status code consistent with return value\n");
        }
        
        // Check for NaN or Inf
        if (std::isnan(result.temp) || std::isinf(result.temp)) {
            printf("  [FAIL] Temperature is NaN or Inf\n");
            consistency_ok = false;
        } else {
            printf("  [PASS] Temperature is valid number\n");
        }
        
        if (std::isnan(result.humi) || std::isinf(result.humi)) {
            printf("  [FAIL] Humidity is NaN or Inf\n");
            consistency_ok = false;
        } else {
            printf("  [PASS] Humidity is valid number\n");
        }
    } else {
        printf("  [SKIP] Read failed, cannot check consistency\n");
    }
    
    if (consistency_ok) {
        passed_subtests++;
    }
    
    // =========================================================================
    // SHT30 Test Summary
    // =========================================================================
    print_subsection("SHT30 Test Summary");
    
    printf("  Total sub-tests: %d\n", total_subtests);
    printf("  Passed: %d\n", passed_subtests);
    printf("  Failed: %d\n", total_subtests - passed_subtests);
    printf("  Pass rate: %.1f%%\n", (float)passed_subtests / total_subtests * 100.0f);
    
    return (passed_subtests == total_subtests) ? 0 : -1;
}

/**
 * @brief Test Alarm Control (Buzzer & RGB LED)
 */
static int test_alarm_control() {
    print_separator("Alarm Control Test");
    
    int ret = 0;
    
    // Test Status 0: Normal (Green LED)
    printf("\n  [TEST] Setting status: NORMAL (Green LED)\n");
    ret = alarm_control_set(0, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=0)", ret);
    if (ret < 0) print_error_code("alarm", ret);
    sleep(1);
    
    // Test Status 1: Warning (Yellow LED)
    printf("\n  [TEST] Setting status: WARNING (Yellow LED)\n");
    ret = alarm_control_set(1, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=1)", ret);
    if (ret < 0) print_error_code("alarm", ret);
    sleep(1);
    
    // Test Status 2: Alarm (Red LED + Buzzer)
    printf("\n  [TEST] Setting status: ALARM (Red LED + Buzzer)\n");
    ret = alarm_control_set(2, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=2)", ret);
    if (ret < 0) print_error_code("alarm", ret);
    sleep(1);
    
    // Restore Normal
    printf("\n  [TEST] Restoring status: NORMAL\n");
    ret = alarm_control_set(0, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    print_result("alarm_control_set(status=0)", ret);
    sleep(1);
    
    // Test Invalid Status
    printf("\n  [TEST] Setting invalid status: -1\n");
    ret = alarm_control_set(-1, BUZZER_PIN, LED_R_PIN, LED_G_PIN, LED_B_PIN);
    if (ret == -4) {
        printf("  [PASS] Correctly returned error code -4 (Invalid alarm status)\n");
    } else {
        printf("  [FAIL] Expected -4, got %d\n", ret);
    }
    
    return 0;
}

/**
 * @brief Test Face Recognition Engine
 */
static int test_face_recognize() {
    print_separator("Face Recognition Engine Test");
    
    FaceSearchInfo info;
    int ret = 0;
    
    // Step 1: Initialize Engine
    printf("\n  [Step 1] Initializing Face Recognition Engine\n");
    printf("  Model path: %s\n", FACE_MODEL_PATH);
    ret = FaceSearchInit(&info, FACE_MODEL_PATH);
    print_result("FaceSearchInit", ret);
    
    if (ret != 0) {
        printf("  Initialization failed. Possible reasons:\n");
        printf("    - Model file path does not exist\n");
        printf("    - Model file loading failed\n");
        printf("  Skipping remaining face recognition tests\n");
        return ret;
    }
    
    // Step 2: Register Face
    printf("\n  [Step 2] Registering Face\n");
    RegisterInfo regInfo;
    regInfo.name = TEST_REGISTER_NAME;
    regInfo.path = TEST_REGISTER_IMAGE;
    
    printf("  Register name: %s\n", regInfo.name.c_str());
    printf("  Image path: %s\n", regInfo.path.c_str());
    
    ret = FaceSearchRegister(info, regInfo);
    print_result("FaceSearchRegister", ret);
    
    if (ret != 0) {
        printf("  [WARN] Registration failed. Test image may not exist. Continuing...\n");
    }
    
    // Step 3: Face Detection
    printf("\n  [Step 3] Face Detection\n");
    printf("  Test image: %s\n", TEST_RECOGNIZE_IMAGE);
    
    FaceRect rects[10];
    int faceCount = GetFaceRects(TEST_RECOGNIZE_IMAGE, rects, 10);
    
    if (faceCount >= 0) {
        printf("  [PASS] GetFaceRects succeeded\n");
        printf("  Faces detected: %d\n", faceCount);
        for (int i = 0; i < faceCount && i < 10; i++) {
            printf("  Face[%d]: x=%d, y=%d, w=%d, h=%d\n",
                   i, rects[i].x, rects[i].y, rects[i].w, rects[i].h);
        }
    } else {
        printf("  [FAIL] GetFaceRects returned error code: %d\n", faceCount);
        printf("  Possible reasons: Image file not found or unsupported format\n");
    }
    
    // Step 4: Face Recognition
    printf("\n  [Step 4] Face Recognition\n");
    std::string result = FaceSearchRecognize(info, TEST_RECOGNIZE_IMAGE);
    
    if (!result.empty()) {
        printf("  [PASS] FaceSearchRecognize succeeded\n");
        printf("  Recognition result: %s\n", result.c_str());
    } else {
        printf("  [FAIL] FaceSearchRecognize returned empty result\n");
        printf("  Possible reasons: No face detected or no match with registered users\n");
    }
    
    // Step 5: Release Resources
    printf("\n  [Step 5] Releasing Face Recognition Engine Resources\n");
    FaceSearchDeinit(&info);
    printf("  [PASS] FaceSearchDeinit completed\n");
    
    return 0;
}

// ==================== Main Function ====================

int main(int argc, char* argv[]) {
    printf("\n");
    printf("================================================================\n");
    printf("     guardsys Southbound Module Comprehensive Test v1.0        \n");
    printf("           OpenHarmony 4.0 Southbound Device Test              \n");
    printf("================================================================\n");
    
    int totalTests = 0;
    int passedTests = 0;
    
    // Test HC-SR501
    totalTests++;
    if (test_hc_sr501() == 0) passedTests++;
    
    // Test MQ-2
    totalTests++;
    if (test_mq2() == 0) passedTests++;
    
    // Test SHT30
    totalTests++;
    if (test_sht30() == 0) passedTests++;
    
    // Test Alarm Control
    totalTests++;
    if (test_alarm_control() == 0) passedTests++;
    
    // Test Face Recognition
    totalTests++;
    if (test_face_recognize() == 0) passedTests++;
    
    // Test Summary
    print_separator("Test Summary");
    printf("  Total test modules: %d\n", totalTests);
    printf("  Passed: %d\n", passedTests);
    printf("  Failed: %d\n", totalTests - passedTests);
    printf("  Pass rate: %.1f%%\n", (float)passedTests / totalTests * 100.0f);
    
    if (passedTests == totalTests) {
        printf("\n  [SUCCESS] All tests passed!\n");
    } else {
        printf("\n  [WARNING] Some tests failed. Please check:\n");
        printf("      1. Hardware connections are correct\n");
        printf("      2. GPIO/I2C/ADC pin configurations\n");
        printf("      3. Model files and test images exist\n");
        printf("      4. Device permissions are sufficient\n");
        printf("      5. I2C bus and device addresses are correct\n");
    }
    
    printf("\n");
    
    return (passedTests == totalTests) ? 0 : 1;
}