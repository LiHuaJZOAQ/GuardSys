//to check , without any verification

/**
 * @file face_recognize_napi.cpp
 * @brief 人脸识别 NAPI 异步桥接实现
 * @details 遵循 guardsys 统一异步架构，将 OpenCV+SeetaFace2 人脸识别操作封装为 ArkTS Promise 接口。
 */

#include "guardsys_napi.h"
#include "face_recognize.h"

namespace guardsys::napi {

// ================= 模块描述符导出 =================

napi_property_descriptor g_face_desc[] = {
    
};
size_t g_face_desc_count = sizeof(g_face_desc) / sizeof(g_face_desc[0]);

} // namespace guardsys::napi