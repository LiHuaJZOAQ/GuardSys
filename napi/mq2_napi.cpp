//to do
/**
@file mq2_napi.cpp
@brief MQ-2 烟雾传感器 NAPI 接口 (适配 Guardsys 架构)
@details 
- 继承 AsyncBaseContext 复用异步生命周期管理
- 使用 QueueAsyncWork 统一提交异步任务
- 导出 g_mq2_desc/g_mq2_desc_count 供 guardsys_napi.cpp 聚合
- 移除独立模块注册，由 guardsys 中枢统一导出
*/
#include "guardsys_napi.h"  // 包含公共头文件，获取 AsyncBaseContext/QueueAsyncWork
#include "sensor_mq2.h"     // 底层驱动接口

namespace guardsys::napi {

/**
@brief MQ-2 异步上下文
@details 继承 AsyncBaseContext，复用 async_work/deferred/status_code
         添加 smoke_value 存储成功时的浓度值
*/
struct Mq2Data : public AsyncBaseContext {
    int adcChannel = 0;      // 输入参数：ADC 通道号
    double smoke_value = 0.0; // 输出结果：烟雾浓度（ppm）
};

/**
@brief 工作线程执行回调
@details 调用阻塞式底层函数，区分成功/失败写入上下文
*/
static void GetMq2DataExecuteCB(napi_env env, void* data)
{
    Mq2Data* ctx = reinterpret_cast<Mq2Data*>(data);
    
    // 调用底层驱动
    int ret = get_mq2_smoke_ppm(ctx->adcChannel);
    
    if (ret < 0) {
        // 失败：错误码写入基类 status_code
        ctx->status_code = ret;
    } else {
        // 成功：状态码为 0，浓度值存入派生类成员
        ctx->status_code = 0;
        ctx->smoke_value = static_cast<double>(ret);
    }
}

/**
@brief 主线程完成回调
@details 根据 status_code resolve 对象或 reject 错误码
*/
static void GetMq2DataCompleteCB(napi_env env, napi_status status, void* data)
{
    Mq2Data* ctx = reinterpret_cast<Mq2Data*>(data);
    
    if (ctx->status_code == 0) {
        // 成功：构造返回对象 { smoke: number }
        napi_value resultObj = nullptr;
        napi_create_object(env, &resultObj);
        
        napi_value smokeVal = nullptr;
        napi_create_double(env, ctx->smoke_value, &smokeVal);
        napi_set_named_property(env, resultObj, "smoke", smokeVal);
        
        napi_resolve_deferred(env, ctx->deferred, resultObj);
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
@brief getMq2Data 方法入口
@details 解析参数，创建上下文，使用 QueueAsyncWork 提交任务
*/
static napi_value GetMq2Data(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    // 创建 Promise
    napi_value promise = nullptr;
    napi_deferred deferred = nullptr;
    napi_create_promise(env, &deferred, &promise);
    
    // 分配上下文
    Mq2Data* ctx = new Mq2Data();
    ctx->deferred = deferred;
    
    // 提取参数 adcChannel
    napi_get_value_int32(env, args[0], &ctx->adcChannel);
    
    // 使用统一辅助函数提交异步任务
    QueueAsyncWork(env, ctx, "GetMq2Data", GetMq2DataExecuteCB, GetMq2DataCompleteCB);
    
    return promise;
}

// ================= 模块描述符导出 =================
// 供 guardsys_napi.cpp 聚合使用，严禁在此处调用 napi_module_register

napi_property_descriptor g_mq2_desc[] = {
    { "getMq2Data", nullptr, GetMq2Data, nullptr, nullptr, nullptr, napi_default, nullptr }
};

size_t g_mq2_desc_count = sizeof(g_mq2_desc) / sizeof(g_mq2_desc[0]);

} // namespace guardsys::napi
