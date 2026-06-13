#include "face_recognize.h"
#include "hilog/log.h"

#undef LOG_DOMAIN
#define LOG_DOMAIN 0x3200
#undef LOG_TAG
#define LOG_TAG "FaceRecognize"


// 全局单例引擎（避免重复初始化）
static FaceSearchInfo* g_faceSearchInfo = nullptr;

int FaceSearchInit(FaceSearchInfo* info, const char* modelPath) {
    if (info == nullptr) {
        HILOG_ERROR(LOG_CORE, "FaceSearchInit: info is nullptr");
        return -1;
    }
    
    // 避免重复初始化
    if (g_faceSearchInfo != nullptr && g_faceSearchInfo->engine != nullptr) {
        HILOG_INFO(LOG_CORE, "FaceSearchInit: already initialized");
        *info = *g_faceSearchInfo;
        return 0;
    }
    
    std::string modelDir(modelPath);
    
    // 配置模型路径
    seeta::ModelSetting::Device device = seeta::ModelSetting::CPU;
    int id = 0;
    
    seeta::ModelSetting FD_model(modelDir + "fd_2_00.dat", device, id);
    seeta::ModelSetting PD_model(modelDir + "pd_2_00_pts5.dat", device, id);
    seeta::ModelSetting FR_model(modelDir + "fr_2_10.dat", device, id);
    
    try {
        // 创建 FaceEngine（集成检测、关键点、识别）
        info->engine = std::make_shared<seeta::FaceEngine>(FD_model, PD_model, FR_model, 2, 16);
        
        // 设置最小人脸尺寸
        info->engine->FD.set(seeta::FaceDetector::PROPERTY_MIN_FACE_SIZE, 80);
        
        // 清空注册库
        info->GalleryIndexMap.clear();
        
        g_faceSearchInfo = info;
        HILOG_INFO(LOG_CORE, "FaceSearchInit: success");
        return 0;
    } catch (const std::exception& e) {
        HILOG_ERROR(LOG_CORE, "FaceSearchInit: exception %{public}s", e.what());
        return -2;
    }
}

void FaceSearchDeinit(FaceSearchInfo* info) {
    if (info != nullptr) {
        info->engine.reset();
        info->GalleryIndexMap.clear();
        if (g_faceSearchInfo == info) {
            g_faceSearchInfo = nullptr;
        }
        HILOG_INFO(LOG_CORE, "FaceSearchDeinit: success");
    }
}

int FaceSearchRegister(FaceSearchInfo& info, const RegisterInfo& regInfo) {
    if (info.engine == nullptr) {
        HILOG_ERROR(LOG_CORE, "FaceSearchRegister: engine not initialized");
        return -1;
    }
    
    try {
        // 读取图片
        cv::Mat image = cv::imread(regInfo.path);
        if (image.empty()) {
            HILOG_ERROR(LOG_CORE, "FaceSearchRegister: cannot read image %{public}s", regInfo.path.c_str());
            return -2;
        }
        
        seeta::ImageData imageData;
        imageData.data = image.data;
        imageData.width = image.cols;
        imageData.height = image.rows;
        imageData.channels = image.channels();   

        // 注册人脸
        auto id = info.engine->Register(imageData);
        if (id >= 0) {
            info.GalleryIndexMap.insert(std::make_pair(id, regInfo.name));
            HILOG_INFO(LOG_CORE, "FaceSearchRegister: %{public}s registered with id %{public}lld", 
                       regInfo.name.c_str(), id);
            return 0;
        } else {
            HILOG_ERROR(LOG_CORE, "FaceSearchRegister: failed to register %{public}s", regInfo.name.c_str());
            return -3;
        }
    } catch (const std::exception& e) {
        HILOG_ERROR(LOG_CORE, "FaceSearchRegister: exception %{public}s", e.what());
        return -4;
    }
}

std::string FaceSearchRecognize(FaceSearchInfo& info, const std::string& imagePath) {
    if (info.engine == nullptr) {
        HILOG_ERROR(LOG_CORE, "FaceSearchRecognize: engine not initialized");
        return "recognize error: engine not initialized";
    }
    
    try {
        cv::Mat frame = cv::imread(imagePath);
        if (frame.empty()) {
            HILOG_ERROR(LOG_CORE, "FaceSearchRecognize: cannot read image %{public}s", imagePath.c_str());
            return "recognize error: cannot read image";
        }
        
        seeta::ImageData image;
        image.data = frame.data;
        image.width = frame.cols;
        image.height = frame.rows;
        image.channels = frame.channels();

        seeta::QualityAssessor QA;
        float threshold = 0.7f;
        
        // 检测人脸
        std::vector<SeetaFaceInfo> faces = info.engine->DetectFaces(image);

        for (const auto& face : faces) {
            // 检测关键点
            auto points = info.engine->DetectPoints(image, face);
            
            // 质量评估
            auto score = QA.evaluate(image, face.pos, points.data());
            if (score == 0) {
                continue; // 质量不合格
            }
            
            // 查询最相似人脸
            int64_t index = 0;
            float similarity = 0.0f;
            auto queried = info.engine->QueryTop(image, points.data(), 1, &index, &similarity);
            
            if (queried < 1) {
                continue;
            }
            
            // 相似度大于阈值，认为识别成功
            if (similarity > threshold) {
                auto it = info.GalleryIndexMap.find(index);
                if (it != info.GalleryIndexMap.end()) {
                    HILOG_INFO(LOG_CORE, "FaceSearchRecognize: %{public}s (similarity: %{public}.2f)", 
                               it->second.c_str(), similarity);
                    return it->second;
                }
            }
        }
        
        HILOG_INFO(LOG_CORE, "FaceSearchRecognize: no match found");
        return "ignored";
    } catch (const std::exception& e) {
        HILOG_ERROR(LOG_CORE, "FaceSearchRecognize: exception %{public}s", e.what());
        return "recognize error: exception";
    }
}

int GetFaceRects(const std::string& imagePath, FaceRect* rects, int maxRects) {
    if (rects == nullptr || maxRects <= 0) {
        HILOG_ERROR(LOG_CORE, "GetFaceRects: invalid parameters");
        return -1;
    }
    
    try {
        cv::Mat frame = cv::imread(imagePath);
        if (frame.empty()) {
            HILOG_ERROR(LOG_CORE, "GetFaceRects: cannot read image %{public}s", imagePath.c_str());
            return -2;
        }
        
        seeta::ImageData image;
        image.data = frame.data;
        image.width = frame.cols;
        image.height = frame.rows;
        image.channels = frame.channels();
        
        // 使用全局引擎或临时创建检测器
        seeta::ModelSetting::Device device = seeta::ModelSetting::CPU;
        seeta::ModelSetting FD_model("/system/usr/model/fd_2_00.dat", device, 0);
        seeta::FaceDetector FD(FD_model);
        FD.set(seeta::FaceDetector::PROPERTY_MIN_FACE_SIZE, 80);
        
        auto faces = FD.detect(image);
        
        int count = 0;
        for (int i = 0; i < faces.size && count < maxRects; i++) {
            rects[count].x = faces.data[i].pos.x;
            rects[count].y = faces.data[i].pos.y;
            rects[count].w = faces.data[i].pos.width;
            rects[count].h = faces.data[i].pos.height;
            count++;
        }
        
        HILOG_INFO(LOG_CORE, "GetFaceRects: detected %{public}d faces", count);
        return count;
    } catch (const std::exception& e) {
        HILOG_ERROR(LOG_CORE, "GetFaceRects: exception %{public}s", e.what());
        return -3;
    }
}
