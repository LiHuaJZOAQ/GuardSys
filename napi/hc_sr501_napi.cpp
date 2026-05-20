/**
 * @file hc_sr501_napi.cpp
 * @brief HC-SR501 NAPI 异步桥接实现
 */
#include "guardsys_napi.h"
#include "sensor_hc_sr501.h"

namespace guardsys::napi {

// 异步上下文结构体
struct Sr501Context : public AsyncBaseContext {
    int gpio_pin = 0;
    bool ir_status = false;
};

// Work线程：执行业务逻辑
static void GetIrStatusExecute(napi_env env, void* data) {
    Sr501Context* ctx = static_cast<Sr501Context*>(data);
    ctx->status_code = sr501_get_status(ctx->gpio_pin, &ctx->ir_status);
}

// 主线程：完成回调，Resolve/Reject Promise
static void GetIrStatusComplete(napi_env env, napi_status status, void* data) {
    Sr501Context* ctx = static_cast<Sr501Context*>(data);
    napi_value result = nullptr;

    if (ctx->status_code == 0) {
        // 创建返回对象 { ir: boolean }
        napi_create_object(env, &result);
        napi_value ir_val;
        napi_get_boolean(env, ctx->ir_status, &ir_val);
        napi_set_named_property(env, result, "ir", ir_val);
        napi_resolve_deferred(env, ctx->deferred, result);
    } else {
        // 错误处理
        napi_value err_msg;
        napi_create_string_utf8(env, "Failed to read HC-SR501 status", NAPI_AUTO_LENGTH, &err_msg);
        napi_reject_deferred(env, ctx->deferred, err_msg);
    }

    // 清理资源
    napi_delete_async_work(env, ctx->async_work);
    delete ctx;
}

// JS 调用入口
static napi_value GetIrStatus(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value argv[1];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    // 参数检查
    if (argc < 1) {
        HILOG_ERROR(LOG_CORE, "getIrStatus: Missing gpioPin argument");
        return nullptr;
    }

    // 创建上下文
    auto ctx = new Sr501Context();
    napi_value promise;
    napi_create_promise(env, &ctx->deferred, &promise);

    // 解析参数
    napi_get_value_int32(env, argv[0], &ctx->gpio_pin);

    // 提交异步任务
    QueueAsyncWork(env, ctx, "GuardsysGetIrStatus", GetIrStatusExecute, GetIrStatusComplete);

    return promise;
}

// 模块描述符
napi_property_descriptor g_sr501_desc[] = {
    { "getIrStatus", nullptr, GetIrStatus, nullptr, nullptr, nullptr, napi_default, nullptr }
};
size_t g_sr501_desc_count = sizeof(g_sr501_desc) / sizeof(g_sr501_desc[0]);

} // namespace guardsys::napi
