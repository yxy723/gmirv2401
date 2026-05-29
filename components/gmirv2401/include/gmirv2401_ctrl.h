/**
 * @file    gmirv2401_ctrl.h
 * @brief   GMIRV2401 设备控制 API - 空调/电视/机顶盒/红外学习/BLE
 */

#ifndef GMIRV2401_CTRL_H
#define GMIRV2401_CTRL_H

#include "gmirv2401.h"
#include "gmirv2401_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * 电池/匹配
 * ================================================================ */

/**
 * @brief 启动空调一键匹配
 *
 * 阻塞调用，等待匹配结果。
 * @param out_code 匹配成功后返回的码库代号 (0x0000-0xFFFF)
 * @param timeout_ms 匹配超时时间 (芯片匹配通常需要数秒)
 * @return ESP_OK 成功，ESP_ERR_TIMEOUT 超时
 */
esp_err_t gmirv2401_ac_auto_match(gmirv2401_t *handle,
                                  uint16_t *out_code,
                                  uint32_t timeout_ms);

/* ================================================================
 * 空调单一状态控制
 * ================================================================ */

/** 设置空调电源: 1=开, 0=关 */
esp_err_t gmirv2401_ac_set_power(gmirv2401_t *handle, uint16_t power);

/** 获取空调电源状态 */
esp_err_t gmirv2401_ac_get_power(gmirv2401_t *handle, uint16_t *power);

/** 设置空调温度: 16-30℃ (寄存器值 0-E) */
esp_err_t gmirv2401_ac_set_temp(gmirv2401_t *handle, uint16_t temp_celsius);

/** 获取空调温度 */
esp_err_t gmirv2401_ac_get_temp(gmirv2401_t *handle, uint16_t *temp_celsius);

/** 设置空调模式: 0-自动 1-制冷 2-除湿 3-送风 4-制热 */
esp_err_t gmirv2401_ac_set_mode(gmirv2401_t *handle, uint16_t mode);

/** 获取空调模式 */
esp_err_t gmirv2401_ac_get_mode(gmirv2401_t *handle, uint16_t *mode);

/** 设置空调风速: 0-自动 1-一档 2-二档 3-三档 */
esp_err_t gmirv2401_ac_set_fan(gmirv2401_t *handle, uint16_t fan);

/** 获取空调风速 */
esp_err_t gmirv2401_ac_get_fan(gmirv2401_t *handle, uint16_t *fan);

/** 设置空调灯光: 0-关灯 1-开灯 */
esp_err_t gmirv2401_ac_set_light(gmirv2401_t *handle, uint16_t light);

/** 获取空调灯光 */
esp_err_t gmirv2401_ac_get_light(gmirv2401_t *handle, uint16_t *light);

/* ================================================================
 * 空调组合状态控制
 * ================================================================ */

/**
 * @brief 空调组合状态参数
 */
typedef struct {
    uint16_t power;     /**< 0-关 1-开 */
    uint16_t swing;     /**< 0-自动风向 1-手动风向 */
    uint16_t light;     /**< 0-关灯 1-开灯 */
    uint16_t temp;      /**< 16-30 ℃ */
    uint16_t mode;      /**< 0-自动 1-制冷 2-除湿 3-送风 4-制热 */
    uint16_t fan;       /**< 0-自动 1-一档 2-二档 3-三档 */
    uint16_t code;      /**< 码库代号 (0xFFFF 代表使用默认) */
    bool     use_specified_code; /**< true=使用指定码库代号 */
    bool     use_single_key;     /**< true=单一按键, false=组合键 */
    uint8_t  key_value;  /**< 键值(当 use_single_key=true): 0-电源 1-风向 3-温度+ 4-温度- 5-模式 6-风速 7-灯光 */
} gmirv2401_ac_state_t;

/** 设置空调组合状态 (同时设置电源/温度/模式/风速等) */
esp_err_t gmirv2401_ac_set_combo(gmirv2401_t *handle,
                                 const gmirv2401_ac_state_t *state);

/** 获取空调组合状态 */
esp_err_t gmirv2401_ac_get_combo(gmirv2401_t *handle,
                                 gmirv2401_ac_state_t *out_state);

/** 获取空调码库代号 */
esp_err_t gmirv2401_ac_get_code(gmirv2401_t *handle, uint16_t *code);

/** 设置空调码库代号 */
esp_err_t gmirv2401_ac_set_code(gmirv2401_t *handle, uint16_t code);

/* ================================================================
 * 电视控制
 * ================================================================ */

/** 电视电源 (翻转键) */
esp_err_t gmirv2401_tv_power(gmirv2401_t *handle);

/** 电视音量+: 发送一次 */
esp_err_t gmirv2401_tv_vol_up(gmirv2401_t *handle);

/** 电视音量-: 发送一次 */
esp_err_t gmirv2401_tv_vol_down(gmirv2401_t *handle);

/** 电视频道+: 发送一次 */
esp_err_t gmirv2401_tv_ch_up(gmirv2401_t *handle);

/** 电视频道-: 发送一次 */
esp_err_t gmirv2401_tv_ch_down(gmirv2401_t *handle);

/** 电视信号源切换 */
esp_err_t gmirv2401_tv_source(gmirv2401_t *handle);

/** 电视返回 */
esp_err_t gmirv2401_tv_return(gmirv2401_t *handle);

/** 获取电视码库代号 */
esp_err_t gmirv2401_tv_get_code(gmirv2401_t *handle, uint16_t *code);

/** 设置电视码库代号 */
esp_err_t gmirv2401_tv_set_code(gmirv2401_t *handle, uint16_t code);

/* ================================================================
 * 机顶盒控制
 * ================================================================ */

/** 机顶盒电源 (翻转键) */
esp_err_t gmirv2401_stb_power(gmirv2401_t *handle);

/** 机顶盒音量+ */
esp_err_t gmirv2401_stb_vol_up(gmirv2401_t *handle);

/** 机顶盒音量- */
esp_err_t gmirv2401_stb_vol_down(gmirv2401_t *handle);

/** 机顶盒频道+ */
esp_err_t gmirv2401_stb_ch_up(gmirv2401_t *handle);

/** 机顶盒频道- */
esp_err_t gmirv2401_stb_ch_down(gmirv2401_t *handle);

/** 机顶盒信号源切换 */
esp_err_t gmirv2401_stb_source(gmirv2401_t *handle);

/** 机顶盒返回 */
esp_err_t gmirv2401_stb_return(gmirv2401_t *handle);

/** 获取机顶盒码库代号 */
esp_err_t gmirv2401_stb_get_code(gmirv2401_t *handle, uint16_t *code);

/** 设置机顶盒码库代号 */
esp_err_t gmirv2401_stb_set_code(gmirv2401_t *handle, uint16_t code);

/* ================================================================
 * 红外学习
 * ================================================================ */

/**
 * @brief 启动红外学习 (通道 1-63)
 *
 * 芯片进入学习模式，灯快闪。
 * 学习完成后芯片会自动上报结果。
 *
 * @param channel 通道号 (1-63)
 * @return ESP_OK 启动成功
 */
esp_err_t gmirv2401_ir_learn_start(gmirv2401_t *handle, uint8_t channel);

/**
 * @brief 退出红外学习
 *
 * @param channel 通道号 (1-63)
 */
esp_err_t gmirv2401_ir_learn_exit(gmirv2401_t *handle, uint8_t channel);

/**
 * @brief 测试学习到的红外波形
 *
 * @param channel 通道号 (1-63)
 */
esp_err_t gmirv2401_ir_learn_test(gmirv2401_t *handle, uint8_t channel);

/**
 * @brief 读取学习到的红外波形数据
 *
 * 每通道 100 字节，占用 50 个寄存器。
 *
 * @param channel  通道号 (1-63)
 * @param out_data 输出缓冲 (至少 100 bytes)
 */
esp_err_t gmirv2401_ir_read_waveform(gmirv2401_t *handle,
                                     uint8_t channel,
                                     uint8_t out_data[100]);

/**
 * @brief 写入红外波形数据
 *
 * @param channel 通道号 (1-63)
 * @param data    波形数据 (100 bytes)
 */
esp_err_t gmirv2401_ir_write_waveform(gmirv2401_t *handle,
                                      uint8_t channel,
                                      const uint8_t data[100]);

/**
 * @brief 查询红外学习状态
 *
 * 轮询寄存器 0016H 来检测是否完成。
 *
 * @param out_status 0x8002=完成, 0x8003=超时, 其他=进行中
 */
esp_err_t gmirv2401_ir_learn_poll_status(gmirv2401_t *handle,
                                         uint16_t *out_status);

/* ================================================================
 * PLC 组态 API (简单寄存器读写)
 * ================================================================ */

esp_err_t gmirv2401_plc_ac_set_power(gmirv2401_t *handle, uint16_t power);
esp_err_t gmirv2401_plc_ac_get_power(gmirv2401_t *handle, uint16_t *power);
esp_err_t gmirv2401_plc_ac_set_temp(gmirv2401_t *handle, uint16_t temp);
esp_err_t gmirv2401_plc_ac_get_temp(gmirv2401_t *handle, uint16_t *temp);
esp_err_t gmirv2401_plc_ac_set_mode(gmirv2401_t *handle, uint16_t mode);
esp_err_t gmirv2401_plc_ac_get_mode(gmirv2401_t *handle, uint16_t *mode);
esp_err_t gmirv2401_plc_ac_set_fan(gmirv2401_t *handle, uint16_t fan);
esp_err_t gmirv2401_plc_ac_get_fan(gmirv2401_t *handle, uint16_t *fan);
esp_err_t gmirv2401_plc_ac_set_light(gmirv2401_t *handle, uint16_t light);
esp_err_t gmirv2401_plc_ac_get_light(gmirv2401_t *handle, uint16_t *light);

/**
 * @brief PLC 方式控制空调发码
 *
 * 写入指定代号到 0041H 触发发码。
 * 若 code=0xFFFF，则使用 0040H 的本地默认代号。
 */
esp_err_t gmirv2401_plc_ac_send(gmirv2401_t *handle, uint16_t code);

/** 获取/设置 PLC 本地默认码库代号 (0040H) */
esp_err_t gmirv2401_plc_ac_get_local_code(gmirv2401_t *handle, uint16_t *code);
esp_err_t gmirv2401_plc_ac_set_local_code(gmirv2401_t *handle, uint16_t code);

/* ================================================================
 * 通用按键发码 (通过组合状态寄存器)
 *
 * 适用于电视、机顶盒的按键控制。
 * 通过写入指定码库代号和设备按键值来发码。
 * ================================================================ */

typedef struct {
    uint8_t  dev_type;  /**< 设备类型: 0-空调 1-电视 2-机顶盒 */
    uint16_t code;      /**< 码库代号 */
    uint8_t  key_id;    /**< 按键 ID (参见 GMIRV2401_KEY_*) */
} gmirv2401_key_send_t;

/**
 * @brief 发送通用按键
 *
 * 此函数通过空调组合状态寄存器来实现任意按键发码。
 * 状态字节 3 填入 key_id，状态字节 4 设置指定码库+单一按键标志。
 */
esp_err_t gmirv2401_send_key(gmirv2401_t *handle,
                             const gmirv2401_key_send_t *key);

/* ================================================================
 * 蓝牙透传
 * ================================================================ */

/**
 * @brief 蓝牙透传 - 发送数据到蓝牙
 *
 * 通过寄存器 3000H 将数据发送到蓝牙端。
 * 格式: [长度 1B] [实际数据 N bytes]
 *
 * @param data      透传数据
 * @param data_len  数据长度 (1-255)
 */
esp_err_t gmirv2401_ble_send(gmirv2401_t *handle,
                             const uint8_t *data, uint8_t data_len);

#ifdef __cplusplus
}
#endif

#endif /* GMIRV2401_CTRL_H */
