//to check , without any verification

/**
 * @file face_recognize_napi.cpp
 * @brief 人脸识别 NAPI 异步桥接实现
 * @details 遵循 guardsys 统一异步架构，将 OpenCV+SeetaFace2 人脸识别操作封装为 ArkTS Promise 接口。
 */

#include "guardsys_napi.h"
#include "face_recognize.h"

namespace guardsys::napi {

// ================= 异步上下文定义 =================

/**
 * @struct FaceAsyncContext
 * @brief 人脸识别专用异步任务上下文
 */
struct FaceAsyncContext : public AsyncBaseContext {
    // 输入参数
    std::string imagePath;
    std::string registerName;
    std::vector<std::string> registerImages;
    int32_t maxFaces = 10;
    
    // 输出结果
    std::vector<FaceRect> faceRects;
    std::string recognizeResult;
    int32_t registerCount = 0;
};

// ================= 全局引擎实例 =================
static FaceSearchInfo g_faceSearchInfo;
static bool g_initialized = false;

// ================= Execute 回调（工作线程） =================

static void FaceInitExecuteCB(napi_env env, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (g_initialized) {
        ctx->status_code = 0;
        return;
    }
    
    ctx->status_code = FaceSearchInit(&g_faceSearchInfo, "/system/usr/model/");
    if (ctx->status_code == 0) {
        g_initialized = true;
    }
}

static void FaceDeinitExecuteCB(napi_env env, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    FaceSearchDeinit(&g_faceSearchInfo);
    g_initialized = false;
    ctx->status_code = 0;
}

static void GetFaceRectsExecuteCB(napi_env env, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (!g_initialized) {
        ctx->status_code = -100; // 未初始化
        return;
    }
    
    FaceRect rects[10];
    int count = GetFaceRects(ctx->imagePath, rects, 10);
    
    if (count >= 0) {
        ctx->status_code = 0;
        for (int i = 0; i < count; i++) {
            ctx->faceRects.push_back(rects[i]);
        }
    } else {
        ctx->status_code = count;
    }
}

static void FaceRegisterExecuteCB(napi_env env, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (!g_initialized) {
        ctx->status_code = -100;
        return;
    }
    
    int successCount = 0;
    for (const auto& path : ctx->registerImages) {
        RegisterInfo regInfo;
        regInfo.name = ctx->registerName;
        regInfo.path = path;
        
        int ret = FaceSearchRegister(g_faceSearchInfo, regInfo);
        if (ret == 0) {
            successCount++;
        }
    }
    
    ctx->registerCount = successCount;
    ctx->status_code = (successCount > 0) ? 0 : -1;
}

static void FaceRecognizeExecuteCB(napi_env env, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (!g_initialized) {
        ctx->status_code = -100;
        return;
    }
    
    ctx->recognizeResult = FaceSearchRecognize(g_faceSearchInfo, ctx->imagePath);
    ctx->status_code = 0;
}

// ================= Complete 回调（主线程） =================

static void FaceInitCompleteCB(napi_env env, napi_status status, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (ctx->status_code == 0) {
        napi_value result;
        napi_create_int32(env, 0, &result);
        napi_resolve_deferred(env, ctx->deferred, result);
    } else {
        napi_value err;
        napi_create_int32(env, ctx->status_code, &err);
        napi_reject_deferred(env, ctx->deferred, err);
    }
    
    napi_delete_async_work(env, ctx->async_work);
    delete ctx;
}

static void GetFaceRectsCompleteCB(napi_env env, napi_status status, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (ctx->status_code == 0) {
        napi_value resultArray;
        napi_create_array(env, &resultArray);
        
        for (size_t i = 0; i < ctx->faceRects.size(); i++) {
            napi_value rectObj;
            napi_create_object(env, &rectObj);
            
            napi_value x, y, w, h;
            napi_create_int32(env, ctx->faceRects[i].x, &x);
            napi_create_int32(env, ctx->faceRects[i].y, &y);
            napi_create_int32(env, ctx->faceRects[i].w, &w);
            napi_create_int32(env, ctx->faceRects[i].h, &h);
            
            napi_set_named_property(env, rectObj, "x", x);
            napi_set_named_property(env, rectObj, "y", y);
            napi_set_named_property(env, rectObj, "w", w);
            napi_set_named_property(env, rectObj, "h", h);
            
            napi_set_element(env, resultArray, i, rectObj);
        }
        
        napi_resolve_deferred(env, ctx->deferred, resultArray);
    } else {
        napi_value err;
        napi_create_int32(env, ctx->status_code, &err);
        napi_reject_deferred(env, ctx->deferred, err);
    }
    
    napi_delete_async_work(env, ctx->async_work);
    delete ctx;
}

static void FaceRegisterCompleteCB(napi_env env, napi_status status, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (ctx->status_code == 0) {
        napi_value result;
        napi_create_int32(env, ctx->registerCount, &result);
        napi_resolve_deferred(env, ctx->deferred, result);
    } else {
        napi_value err;
        napi_create_int32(env, ctx->status_code, &err);
        napi_reject_deferred(env, ctx->deferred, err);
    }
    
    napi_delete_async_work(env, ctx->async_work);
    delete ctx;
}

static void FaceRecognizeCompleteCB(napi_env env, napi_status status, void* data) {
    auto* ctx = static_cast<FaceAsyncContext*>(data);
    
    if (ctx->status_code == 0) {
        napi_value result;
        napi_create_string_utf8(env, ctx->recognizeResult.c_str(), NAPI_AUTO_LENGTH, &result);
        napi_resolve_deferred(env, ctx->deferred, result);
    } else {
        napi_value err;
        napi_create_int32(env, ctx->status_code, &err);
        napi_reject_deferred(env, ctx->deferred, err);
    }
    
    napi_delete_async_work(env, ctx->async_work);
    delete ctx;
}

// ================= NAPI 导出函数 =================

static napi_value FaceInit(napi_env env, napi_callback_info info) {
    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);
    
    auto* ctx = new FaceAsyncContext();
    ctx->deferred = deferred;
    
    QueueAsyncWork(env, ctx, "FaceInit", FaceInitExecuteCB, FaceInitCompleteCB);
    
    return promise;
}

static napi_value FaceDeinit(napi_env env, napi_callback_info info) {
    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);
    
    auto* ctx = new FaceAsyncContext();
    ctx->deferred = deferred;
    
    QueueAsyncWork(env, ctx, "FaceDeinit", FaceDeinitExecuteCB, FaceInitCompleteCB);
    
    return promise;
}

static napi_value GetFaceRects(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    
    if (argc < 1) {
        HILOG_ERROR(LOG_CORE, "GetFaceRects: missing imagePath parameter");
        return nullptr;
    }
    
    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);
    
    auto* ctx = new FaceAsyncContext();
    ctx->deferred = deferred;
    
    char path[512] = {0};
    size_t len = 0;
    napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &len);
    ctx->imagePath = path;
    
    QueueAsyncWork(env, ctx, "GetFaceRects", GetFaceRectsExecuteCB, GetFaceRectsCompleteCB);
    
    return promise;
}

static napi_value FaceRegister(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    
    if (argc < 1) {
        HILOG_ERROR(LOG_CORE, "FaceRegister: missing params");
        return nullptr;
    }
    
    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);
    
    auto* ctx = new FaceAsyncContext();
    ctx->deferred = deferred;
    
    // 解析参数对象 { name: string, images: string[] }
    napi_value obj = argv[0];
    napi_value value;
    
    // 获取 name
    if (napi_get_named_property(env, obj, "name", &value) == napi_ok) {
        char name[64] = {0};
        size_t len = 0;
        napi_get_value_string_utf8(env, value, name, sizeof(name), &len);
        ctx->registerName = name;
    }
    
    // 获取 images 数组
    if (napi_get_named_property(env, obj, "images", &value) == napi_ok) {
        bool isArray = false;
        napi_is_array(env, value, &isArray);
        if (isArray) {
            uint32_t arrLen = 0;
            napi_get_array_length(env, value, &arrLen);
            for (uint32_t i = 0; i < arrLen; i++) {
                napi_value imgPath;
                napi_get_element(env, value, i, &imgPath);
                char path[512] = {0};
                size_t len = 0;
                napi_get_value_string_utf8(env, imgPath, path, sizeof(path), &len);
                ctx->registerImages.push_back(path);
            }
        }
    }
    
    QueueAsyncWork(env, ctx, "FaceRegister", FaceRegisterExecuteCB, FaceRegisterCompleteCB);
    
    return promise;
}

static napi_value FaceRecognize(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    
    if (argc < 1) {
        HILOG_ERROR(LOG_CORE, "FaceRecognize: missing imagePath parameter");
        return nullptr;
    }
    
    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);
    
    auto* ctx = new FaceAsyncContext();
    ctx->deferred = deferred;
    
    char path[512] = {0};
    size_t len = 0;
    napi_get_value_string_utf8(env, argv[0], path, sizeof(path), &len);
    ctx->imagePath = path;
    
    QueueAsyncWork(env, ctx, "FaceRecognize", FaceRecognizeExecuteCB, FaceRecognizeCompleteCB);
    
    return promise;
}

// ================= 模块描述符导出 =================

napi_property_descriptor g_face_desc[] = {
    DECLARE_NAPI_FUNCTION("faceInit", FaceInit),
    DECLARE_NAPI_FUNCTION("faceDeinit", FaceDeinit),
    DECLARE_NAPI_FUNCTION("getFaceRects", GetFaceRects),
    DECLARE_NAPI_FUNCTION("faceRegister", FaceRegister),
    DECLARE_NAPI_FUNCTION("faceRecognize", FaceRecognize),
};

size_t g_face_desc_count = sizeof(g_face_desc) / sizeof(g_face_desc[0]);

} // namespace guardsys::napi