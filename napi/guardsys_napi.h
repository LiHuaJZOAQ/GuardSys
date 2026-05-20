/**
 * @file guardsys_napi.h
 * @brief Guardsys NAPI 公共头文件
 * @details 定义异步基类、统一队列辅助函数及各子模块描述符外部声明。
 *          所有 *_napi.cpp 业务文件仅需包含此头文件，严禁修改本文件中的基类与辅助函数。
 */

#ifndef GUARDSYS_NAPI_H
#define GUARDSYS_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "hilog/log.h"

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "GUARDSYS_NAPI"

namespace guardsys::napi {

/**
 * @struct AsyncBaseContext
 * @brief 异步任务统一上下文基类
 * @details 封装 Promise deferred 句柄、异步工作项及执行状态。
 *          所有传感器/功能模块的异步上下文必须继承此类，以复用统一的生命周期管理。
 */
struct AsyncBaseContext {
    napi_async_work async_work = nullptr;   /**< NAPI 异步工作项句柄 */
    napi_deferred deferred = nullptr;       /**< Promise 延迟对象 */
    int32_t status_code = 0;                /**< 底层执行状态码（0成功，<0失败） */
    virtual ~AsyncBaseContext() = default;  /**< 虚析构函数确保派生类安全释放 */
};

/**
 * @brief 通用异步工作项创建辅助函数
 * @details 内联函数，零开销封装 napi_create_async_work 与 napi_queue_async_work。
 *          开发者在业务文件中直接调用此函数即可提交异步任务至底层线程池。
 *
 * @param env NAPI 环境上下文
 * @param ctx 异步上下文指针（需继承 AsyncBaseContext）
 * @param work_name 工作项标识名（用于性能分析与日志追踪）
 * @param exec_cb 工作线程回调函数（禁止调用 NAPI 接口）
 * @param complete_cb 主线程回调函数（可安全调用 NAPI 接口）
 */
inline void QueueAsyncWork(napi_env env, AsyncBaseContext* ctx, const char* work_name,
                           napi_async_execute_callback exec_cb,
                           napi_async_complete_callback complete_cb) {
    napi_value res_name;
    napi_create_string_utf8(env, work_name, NAPI_AUTO_LENGTH, &res_name);
    napi_create_async_work(env, nullptr, res_name, exec_cb, complete_cb, ctx, &ctx->async_work);
    napi_queue_async_work(env, ctx->async_work);
}

// ================= 各模块描述符外部声明 =================
// 开发者新增模块时，需在此处追加对应的 extern 声明，格式保持一致
extern napi_property_descriptor g_sht30_desc[];
extern size_t g_sht30_desc_count;

extern napi_property_descriptor g_sr501_desc[];
extern size_t g_sr501_desc_count;

extern napi_property_descriptor g_mq2_desc[];
extern size_t g_mq2_desc_count;

extern napi_property_descriptor g_alarm_desc[];
extern size_t g_alarm_desc_count;

extern napi_property_descriptor g_face_desc[];
extern size_t g_face_desc_count;

} // namespace guardsys::napi

#endif // GUARDSYS_NAPI_H