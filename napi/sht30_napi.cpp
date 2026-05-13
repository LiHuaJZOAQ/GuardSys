/**
 * @file sht30_napi.cpp
 * @brief SHT30 温湿度传感器 NAPI 异步桥接实现
 * @details 遵循 guardsys 统一异步架构，将底层阻塞型 I2C 读取操作封装为 ArkTS Promise 接口。
 *          硬件读取在独立 Worker 线程执行，EventLoop 主线程负责 Promise 决议与 JS 对象构建。
 *          本文件仅包含 SHT30 模块特有逻辑，通过 g_sht30_desc 向中枢注册文件暴露接口。
 */

#include "guardsys_napi.h"
#include "sensor_sht30.h"

namespace guardsys::napi {

/**
 * @struct Sht30AsyncContext
 * @brief SHT30 专用异步任务上下文
 * @details 继承 AsyncBaseContext 基类，复用 deferred 与 async_work 生命周期管理。
 *          仅保留本模块所需的输入参数（总线号、设备地址）与输出结果（温湿度浮点数）。
 *          内存由 new 在堆区分配，生命周期由 CompleteCB 统一回收。
 */
struct Sht30AsyncContext : public AsyncBaseContext {
    int i2c_bus = 0;          /**< I2C 总线号，由 ArkTS 侧传入 */
    uint8_t address = 0;      /**< I2C 设备地址，由 ArkTS 侧传入 */
    float temp = 0.0f;        /**< 温度计算结果，单位：摄氏度 */
    float humi = 0.0f;        /**< 湿度计算结果，单位：百分比 */
};

/**
 * @brief 异步任务执行回调（运行于底层 Worker 线程）
 * @details 负责调用底层 C 驱动接口执行实际硬件通信。
 *          此函数严禁调用任何 NAPI/JS 接口，仅处理纯数据读取、CRC 校验与物理量换算。
 *          执行结果通过 status_code 与 temp/humi 字段传递至完成回调。
 *
 * @param env NAPI 环境指针（仅作占位，禁止直接使用）
 * @param data 指向 Sht30AsyncContext 的用户自定义数据
 */
static void Sht30ExecuteCB(napi_env env, void* data) {
    auto* ctx = static_cast<Sht30AsyncContext*>(data);
    Sht30DataResult result;

    // 调用底层阻塞型 I2C 读取函数（路径: /dev/i2c-x，包含 CRC-8 校验）
    ctx->status_code = sht30_read_data(ctx->i2c_bus, ctx->address, &result);

    if (ctx->status_code == 0) {
        // 读取成功，缓存转换后的物理量至上下文
        ctx->temp = result.temp;
        ctx->humi = result.humi;
    }
    // 失败时 status_code 已携带底层错误码（-1 至 -5），直接透传至 CompleteCB
}

/**
 * @brief 异步任务完成回调（运行于 EventLoop 主线程）
 * @details Worker 线程执行完毕后由框架自动调度至此。
 *          负责将 C 层结果转换为 JS 对象 { temp: number, humi: number }，
 *          并根据 status_code 调用 napi_resolve_deferred 或 napi_reject_deferred。
 *          最后清理异步工作项与上下文堆内存，防止资源泄漏。
 *
 * @param env NAPI 环境指针（可安全调用 NAPI 接口）
 * @param status 异步任务调度状态（通常为 napi_ok）
 * @param data 指向 Sht30AsyncContext 的用户自定义数据
 */
static void Sht30CompleteCB(napi_env env, napi_status status, void* data) {
    auto* ctx = static_cast<Sht30AsyncContext*>(data);

    if (ctx->status_code == 0) {
        // 成功：构造 JS 对象
        napi_value result_obj;
        napi_create_object(env, &result_obj);

        napi_value val_temp, val_humi;
        napi_create_double(env, ctx->temp, &val_temp);
        napi_create_double(env, ctx->humi, &val_humi);

        napi_set_named_property(env, result_obj, "temp", val_temp);
        napi_set_named_property(env, result_obj, "humi", val_humi);

        // 触发 Promise.resolve(result_obj)
        napi_resolve_deferred(env, ctx->deferred, result_obj);
    } else {
        // 失败：构造错误码并触发 Promise.reject(error_code)
        napi_value err_code;
        napi_create_int32(env, ctx->status_code, &err_code);
        napi_reject_deferred(env, ctx->deferred, err_code);
    }

    // 清理 NAPI 异步工作项资源
    napi_delete_async_work(env, ctx->async_work);
    // 释放堆内存
    delete ctx;
}

/**
 * @brief NAPI 导出函数：异步获取 SHT30 实时温湿度
 * @details ArkTS 侧调用此函数将立即返回 Promise 对象。
 *          参数校验、Promise 创建、上下文分配均在此函数完成，随后入队交由底层线程池调度。
 *
 * @param env NAPI 环境指针
 * @param info 回调信息，包含 ArkTS 传入的参数列表 [i2cBus: number, address: number]
 * @return napi_value 返回 Promise 实例供 ArkTS 侧 await 或 .then() 处理
 */
static napi_value GetSht30Data(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    // 参数数量校验
    if (argc < 2) {
        HILOG_ERROR(LOG_CORE, "GetSht30Data: 参数不足，需传入 (i2cBus, address)");
        return nullptr;
    }

    // 参数类型校验
    napi_valuetype type_bus, type_addr;
    napi_typeof(env, argv[0], &type_bus);
    napi_typeof(env, argv[1], &type_addr);
    if (type_bus != napi_number || type_addr != napi_number) {
        HILOG_ERROR(LOG_CORE, "GetSht30Data: 参数类型错误，必须为 number");
        return nullptr;
    }

    // 创建 Promise 对象与 Deferred 句柄
    napi_value promise;
    napi_deferred deferred;
    napi_create_promise(env, &deferred, &promise);

    // 分配异步上下文（堆分配，跨线程安全）
    auto* ctx = new Sht30AsyncContext();
    ctx->deferred = deferred;

    // 解析 ArkTS 传入参数
    int32_t bus_int = 0;
    uint32_t addr_uint = 0;
    napi_get_value_int32(env, argv[0], &bus_int);
    napi_get_value_uint32(env, argv[1], &addr_uint);

    ctx->i2c_bus = bus_int;
    ctx->address = static_cast<uint8_t>(addr_uint & 0xFF);

    // 复用统一异步队列辅助函数，将任务提交至底层线程池
    QueueAsyncWork(env, ctx, "Sht30Read", Sht30ExecuteCB, Sht30CompleteCB);

    // 立即返回 Promise，不阻塞 ArkTS 线程
    return promise;
}

// ================= 模块描述符导出区 =================
/**
 * @brief 暴露本模块 NAPI 属性描述符数组
 * @details 供 guardsys_napi.cpp 的聚合逻辑使用。
 *          数组需为全局链接属性，长度通过 sizeof 计算。
 *          后续若增加 SHT30 配置接口（如 setRefreshRate），可在此追加 DECLARE_NAPI_FUNCTION。
 */
napi_property_descriptor g_sht30_desc[] = {
    DECLARE_NAPI_FUNCTION("getSht30Data", GetSht30Data)
};

size_t g_sht30_desc_count = sizeof(g_sht30_desc) / sizeof(g_sht30_desc[0]);

} // namespace guardsys::napi