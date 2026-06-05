//to check , without any verification
#ifndef FACE_RECOGNIZE_H
#define FACE_RECOGNIZE_H

#include <string>
#include <vector>
#include <memory>
#include <map>

// SeetaFace2 头文件
#include "seeta/FaceDetector.h"
#include "seeta/FaceLandmarker.h"
#include "seeta/FaceRecognizer.h"
#include "seeta/QualityAssessor.h"
#include "seeta/CFaceInfo.h"
#include "../../../third_party/SeetaFace2/example/search/seeta/FaceEngine.h"
#include "opencv2/opencv.hpp"



// 人脸矩形框结构
struct FaceRect {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

// 注册信息结构
struct RegisterInfo {
    std::string name;
    std::string path;
};

// 人脸识别引擎信息
struct FaceSearchInfo {
    std::shared_ptr<seeta::FaceEngine> engine;
    std::map<int64_t, std::string> GalleryIndexMap;
};

// ================= 公共接口 =================

/**
 * @brief 初始化人脸识别引擎
 * @param info 引擎信息结构体指针
 * @param modelPath 模型文件路径（默认 /system/usr/model/）
 * @return 0 成功，<0 失败
 */
int FaceSearchInit(FaceSearchInfo* info, const char* modelPath = "/system/usr/model/");

/**
 * @brief 释放人脸识别引擎资源
 * @param info 引擎信息结构体指针
 */
void FaceSearchDeinit(FaceSearchInfo* info);

/**
 * @brief 注册人脸到识别引擎
 * @param info 引擎信息引用
 * @param regInfo 注册信息（姓名+图片路径）
 * @return 0 成功，<0 失败
 */
int FaceSearchRegister(FaceSearchInfo& info, const RegisterInfo& regInfo);

/**
 * @brief 识别图片中的人脸
 * @param info 引擎信息引用
 * @param imagePath 待识别图片路径
 * @return 识别结果（姓名或错误信息）
 */
std::string FaceSearchRecognize(FaceSearchInfo& info, const std::string& imagePath);

/**
 * @brief 获取图片中的人脸框坐标
 * @param imagePath 图片路径
 * @param rects 输出的人脸框数组
 * @param maxRects 最大人脸框数量
 * @return 检测到的人脸数量，<0 表示错误
 */
int GetFaceRects(const std::string& imagePath, FaceRect* rects, int maxRects);


#endif // FACE_RECOGNIZE_H