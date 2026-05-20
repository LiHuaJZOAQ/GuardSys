/**
 * @file guardsys_napi.cpp
 * @brief Guardsys NAPI 模块注册中枢
 * @details 聚合所有子模块（传感器/执行器/AI）的导出描述符，
 *          统一注册为单一动态库模块供 ArkTS 侧调用。
 */

#include "guardsys_napi.h"

namespace guardsys::napi {

/**
 * @brief NAPI 模块初始化入口
 * @details 运行时聚合各子系统的 napi_property_descriptor 数组，
 *          通过 napi_define_properties 一次性导出至 JS 引擎。
 *          采用安全边界检查，防止数组越界导致引擎崩溃。
 */
static napi_value Init(napi_env env, napi_value exports) {
    // 预留空间容纳所有子模块接口（64 足够支撑 20+ 模块，编译期确定大小）
    constexpr size_t MAX_DESC = 64;
    napi_property_descriptor all_desc[MAX_DESC];
    size_t count = 0;

    // 安全聚合辅助 Lambda
    auto append = [&](const napi_property_descriptor* src, size_t src_cnt) {
        for (size_t i = 0; i < src_cnt && count < MAX_DESC; ++i, ++count) {
            all_desc[count] = src[i];
        }
    };

    // 按模块追加描述符（新增模块仅需在此追加一行）
    append(g_sht30_desc, g_sht30_desc_count);
    append(g_sr501_desc, g_sr501_desc_count);
    append(g_mq2_desc, g_mq2_desc_count);
    append(g_alarm_desc, g_alarm_desc_count);
    append(g_face_desc, g_face_desc_count);

    // 统一导出至 ArkTS 侧
    napi_define_properties(env, exports, count, all_desc);
    return exports;
}

} // namespace guardsys::napi

/**
 * @brief NAPI 模块注册结构体
 * @details 指定模块名为 "guardsys"，ArkTS 侧通过 import guardsys from '@ohos.guardsys' 引入。
 */
static napi_module guardsysModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = guardsys::napi::Init,
    .nm_modname = "guardsys",
    .nm_priv = nullptr,
    .reserved = {0},
};

/**
 * @brief 动态库加载自动注册函数
 * @details 利用 constructor 属性在 so 库被 dlopen 加载时自动调用 napi_module_register。
 */
extern "C" __attribute__((constructor)) void RegisterGuardsysModule(void) {
    napi_module_register(&guardsysModule);
}