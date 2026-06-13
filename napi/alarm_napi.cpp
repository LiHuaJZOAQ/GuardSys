/**
@file alarm_napi.cpp
@brief 报警声光控制 NAPI 接口 (适配 Guardsys 架构)
@details 
- 继承 AsyncBaseContext 复用异步生命周期管理
- 使用 QueueAsyncWork 统一提交异步任务
- 导出 g_alarm_desc/g_alarm_desc_count 供 guardsys_napi.cpp 聚合
- 移除独立模块注册，由 guardsys 中枢统一导出
*/
#include "guardsys_napi.h"  // 包含公共头文件，获取 AsyncBaseContext/QueueAsyncWork
#include "alarm_control.h"  // 底层控制接口

namespace guardsys::napi {

/**
@brief 报警控制异步上下文
@details 继承 AsyncBaseContext，复用 async_work/deferred/status_code
*/
struct AlarmData : public AsyncBaseContext {
    // 输入参数
    int status = 0;
    int buzzerPin = 0;
    int rPin = 0;
    int gPin = 0;
    int bPin = 0;
};

/**
@brief 工作线程执行回调
@details 调用阻塞式底层函数，结果写入基类 status_code
*/
static void SetAlarmExecuteCB(napi_env env, void* data)
{
    AlarmData* ctx = reinterpret_cast<AlarmData*>(data);
    // 调用底层驱动，结果存入基类 status_code
    ctx->status_code = alarm_control_set(
        ctx->status,
        ctx->buzzerPin,
        ctx->rPin,
        ctx->gPin,
        ctx->bPin
    );
}

/**
@brief 主线程完成回调
@details 根据 status_code resolve/reject，并清理资源
*/
static void SetAlarmCompleteCB(napi_env env, napi_status status, void* data)
{
    AlarmData* ctx = reinterpret_cast<AlarmData*>(data);
    
    if (ctx->status_code == 0) {
        // 成功：resolve undefined
        napi_value undefined = nullptr;
        napi_get_undefined(env, &undefined);
        napi_resolve_deferred(env, ctx->deferred, undefined);
    } else {
        // 失败：reject 错误码
        napi_value errorCode = nullptr;
        napi_create_int32(env, ctx->status_code, &errorCode);
        napi_reject_deferred(env, ctx->deferred, errorCode);
    }
    
    // 清理资源
    napi_delete_async_work(env, ctx->async_work);
    delete ctx;
}

/**
@brief setAlarmStatus 方法入口
@details 解析参数，创建上下文，使用 QueueAsyncWork 提交任务
*/
static napi_value SetAlarmStatus(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    // 创建 Promise
    napi_value promise = nullptr;
    napi_deferred deferred = nullptr;
    napi_create_promise(env, &deferred, &promise);

    // 分配上下文
    AlarmData* ctx = new AlarmData();
    ctx->deferred = deferred;

    // 提取参数 status
    napi_get_value_int32(env, args[0], &ctx->status);

    // 提取参数 pins 对象
    napi_value pinObj = args[1];
    napi_value val;
    
    napi_get_named_property(env, pinObj, "buzzerPin", &val);
    napi_get_value_int32(env, val, &ctx->buzzerPin);
    
    napi_get_named_property(env, pinObj, "rPin", &val);
    napi_get_value_int32(env, val, &ctx->rPin);
    
    napi_get_named_property(env, pinObj, "gPin", &val);
    napi_get_value_int32(env, val, &ctx->gPin);
    
    napi_get_named_property(env, pinObj, "bPin", &val);
    napi_get_value_int32(env, val, &ctx->bPin);

    // 使用统一辅助函数提交异步任务
    QueueAsyncWork(env, ctx, "SetAlarmStatus", SetAlarmExecuteCB, SetAlarmCompleteCB);

    return promise;
}

// ================= 模块描述符导出 =================
// 供 guardsys_napi.cpp 聚合使用，严禁在此处调用 napi_module_register

napi_property_descriptor g_alarm_desc[] = {
    { "setAlarmStatus", nullptr, SetAlarmStatus, nullptr, nullptr, nullptr, napi_default, nullptr }
};

size_t g_alarm_desc_count = sizeof(g_alarm_desc) / sizeof(g_alarm_desc[0]);

} // namespace guardsys::napi
