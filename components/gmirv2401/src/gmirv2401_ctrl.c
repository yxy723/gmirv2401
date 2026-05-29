/**
 * @file    gmirv2401_ctrl.c
 * @brief   GMIRV2401 设备控制实现 - 空调/电视/机顶盒/红外学习/BLE
 */

#include "gmirv2401_ctrl.h"
#include "gmirv2401_reg.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GMIRV2401_CTRL";

/* ================================================================
 * 工具宏
 * ================================================================ */

#define WRITE_REG(h, reg, val) \
    gmirv2401_write_register((h), (reg), (val), 0)

#define READ_REG(h, reg, val) \
    gmirv2401_read_u16((h), (reg), (val), 0)

/* ================================================================
 * 空调一键匹配
 * ================================================================ */

esp_err_t gmirv2401_ac_auto_match(gmirv2401_t *handle,
                                  uint16_t *out_code,
                                  uint32_t timeout_ms)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    if (timeout_ms == 0) timeout_ms = 30000; /* 默认 30 秒 */

    /* 启动匹配 */
    esp_err_t err = gmirv2401_write_register(handle, GMIRV2401_REG_AC_MATCH,
                                             0x0001, 500);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ac_match start failed: %d", err);
        return err;
    }

    ESP_LOGI(TAG, "AC auto-match started, waiting...");

    /* 轮询匹配结果 */
    uint32_t elapsed = 0;
    uint32_t poll_interval = 500; /* 500ms 轮询一次 */

    while (elapsed < timeout_ms) {
        vTaskDelay(pdMS_TO_TICKS(poll_interval));
        elapsed += poll_interval;

        uint16_t result = 0;
        err = gmirv2401_read_u16(handle, GMIRV2401_REG_AC_MATCH, &result, 500);
        if (err != ESP_OK) continue;

        if (result == GMIRV2401_MATCH_TIMEOUT) {
            ESP_LOGW(TAG, "AC match timeout/error");
            return ESP_ERR_TIMEOUT;
        }

        if (result != GMIRV2401_MATCH_START) {
            /* 匹配成功 - result 即为码库代号 */
            ESP_LOGI(TAG, "AC match success: code=0x%04X", result);
            if (out_code) *out_code = result;
            return ESP_OK;
        }
    }

    ESP_LOGW(TAG, "AC match poll timeout");
    return ESP_ERR_TIMEOUT;
}

/* ================================================================
 * 空调单一状态控制
 * ================================================================ */

esp_err_t gmirv2401_ac_set_power(gmirv2401_t *handle, uint16_t power)
{
    return WRITE_REG(handle, GMIRV2401_REG_AC_POWER, power);
}

esp_err_t gmirv2401_ac_get_power(gmirv2401_t *handle, uint16_t *power)
{
    return READ_REG(handle, GMIRV2401_REG_AC_POWER, power);
}

esp_err_t gmirv2401_ac_set_temp(gmirv2401_t *handle, uint16_t temp_celsius)
{
    if (temp_celsius < 16 || temp_celsius > 30) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_AC_TEMP, temp_celsius - 16);
}

esp_err_t gmirv2401_ac_get_temp(gmirv2401_t *handle, uint16_t *temp_celsius)
{
    uint16_t val;
    esp_err_t err = READ_REG(handle, GMIRV2401_REG_AC_TEMP, &val);
    if (err == ESP_OK && temp_celsius) *temp_celsius = val + 16;
    return err;
}

esp_err_t gmirv2401_ac_set_mode(gmirv2401_t *handle, uint16_t mode)
{
    if (mode > 4) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_AC_MODE, mode);
}

esp_err_t gmirv2401_ac_get_mode(gmirv2401_t *handle, uint16_t *mode)
{
    return READ_REG(handle, GMIRV2401_REG_AC_MODE, mode);
}

esp_err_t gmirv2401_ac_set_fan(gmirv2401_t *handle, uint16_t fan)
{
    if (fan > 3) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_AC_FAN, fan);
}

esp_err_t gmirv2401_ac_get_fan(gmirv2401_t *handle, uint16_t *fan)
{
    return READ_REG(handle, GMIRV2401_REG_AC_FAN, fan);
}

esp_err_t gmirv2401_ac_set_light(gmirv2401_t *handle, uint16_t light)
{
    if (light > 1) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_AC_LIGHT, light);
}

esp_err_t gmirv2401_ac_get_light(gmirv2401_t *handle, uint16_t *light)
{
    return READ_REG(handle, GMIRV2401_REG_AC_LIGHT, light);
}

/* ================================================================
 * 空调组合状态控制
 * ================================================================ */

esp_err_t gmirv2401_ac_set_combo(gmirv2401_t *handle,
                                 const gmirv2401_ac_state_t *state)
{
    if (!handle || !state) return ESP_ERR_INVALID_ARG;

    uint8_t data[6];

    /* 002AH: 代码号 (高位在前) */
    data[0] = (uint8_t)(state->code >> 8);
    data[1] = (uint8_t)(state->code & 0xFF);

    /* 002BH: 状态字节1 + 状态字节2 */
    uint8_t state1 = 0;
    state1 |= (state->power & 0x01) << 7;    /* Bit7: 开关 */
    state1 |= (state->swing & 0x01) << 6;    /* Bit6: 风向 */
    state1 |= (state->light & 0x01) << 5;    /* Bit5: 灯光 */
    /* Bit4: 保留 0 */
    uint16_t temp_val = (state->temp >= 16) ? (state->temp - 16) : 0;
    state1 |= (uint8_t)(temp_val & 0x0F);    /* Bit3-0: 温度 0-E */

    uint8_t state2 = 0;
    state2 |= (uint8_t)((state->mode & 0x0F) << 4);  /* Bit7-4: 模式 */
    state2 |= (uint8_t)(state->fan & 0x0F);           /* Bit3-0: 风速 */

    data[2] = state1;
    data[3] = state2;

    /* 002CH: 状态字节3 + 状态字节4 */
    data[4] = state->key_value;  /* 状态字节3: 键值 */
    uint8_t state4 = 0;
    if (state->use_single_key)  state4 |= 0x02; /* Bit1: 单一按键 */
    if (state->use_specified_code) state4 |= 0x01; /* Bit0: 指定码库 */
    data[5] = state4;

    ESP_LOGD(TAG, "ac_combo: code=%04X s1=%02X s2=%02X s3=%02X s4=%02X",
             state->code, state1, state2, data[4], state4);

    return gmirv2401_write_registers(handle, GMIRV2401_REG_AC_COMBO_CODE,
                                     3, data, 0);
}

esp_err_t gmirv2401_ac_get_combo(gmirv2401_t *handle,
                                 gmirv2401_ac_state_t *out_state)
{
    if (!handle || !out_state) return ESP_ERR_INVALID_ARG;

    uint8_t data[6];
    esp_err_t err = gmirv2401_read_registers(handle, GMIRV2401_REG_AC_COMBO_CODE,
                                             3, data, 0);
    if (err != ESP_OK) return err;

    memset(out_state, 0, sizeof(*out_state));

    out_state->code = ((uint16_t)data[0] << 8) | data[1];

    uint8_t s1 = data[2];
    out_state->power = (s1 >> 7) & 0x01;
    out_state->swing = (s1 >> 6) & 0x01;
    out_state->light = (s1 >> 5) & 0x01;
    out_state->temp  = (s1 & 0x0F) + 16;

    uint8_t s2 = data[3];
    out_state->mode = (s2 >> 4) & 0x0F;
    out_state->fan  = s2 & 0x0F;

    uint8_t s4 = data[5];
    out_state->key_value          = data[4];
    out_state->use_single_key     = (s4 & 0x02) ? true : false;
    out_state->use_specified_code = (s4 & 0x01) ? true : false;

    return ESP_OK;
}

esp_err_t gmirv2401_ac_get_code(gmirv2401_t *handle, uint16_t *code)
{
    return READ_REG(handle, GMIRV2401_REG_AC_CODE, code);
}

esp_err_t gmirv2401_ac_set_code(gmirv2401_t *handle, uint16_t code)
{
    return WRITE_REG(handle, GMIRV2401_REG_AC_CODE, code);
}

/* ================================================================
 * 电视控制
 * ================================================================ */

esp_err_t gmirv2401_tv_power(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_POWER, 0x0001);
}

esp_err_t gmirv2401_tv_vol_up(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_VOLUME, 0x0001);
}

esp_err_t gmirv2401_tv_vol_down(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_VOLUME, 0x0000);
}

esp_err_t gmirv2401_tv_ch_up(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_CHANNEL, 0x0001);
}

esp_err_t gmirv2401_tv_ch_down(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_CHANNEL, 0x0000);
}

esp_err_t gmirv2401_tv_source(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_SOURCE, 0x0001);
}

esp_err_t gmirv2401_tv_return(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_RETURN, 0x0001);
}

esp_err_t gmirv2401_tv_get_code(gmirv2401_t *handle, uint16_t *code)
{
    return READ_REG(handle, GMIRV2401_REG_TV_CODE, code);
}

esp_err_t gmirv2401_tv_set_code(gmirv2401_t *handle, uint16_t code)
{
    return WRITE_REG(handle, GMIRV2401_REG_TV_CODE, code);
}

/* ================================================================
 * 机顶盒控制
 * ================================================================ */

esp_err_t gmirv2401_stb_power(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_POWER, 0x0001);
}

esp_err_t gmirv2401_stb_vol_up(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_VOLUME, 0x0001);
}

esp_err_t gmirv2401_stb_vol_down(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_VOLUME, 0x0000);
}

esp_err_t gmirv2401_stb_ch_up(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_CHANNEL, 0x0001);
}

esp_err_t gmirv2401_stb_ch_down(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_CHANNEL, 0x0000);
}

esp_err_t gmirv2401_stb_source(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_SOURCE, 0x0001);
}

esp_err_t gmirv2401_stb_return(gmirv2401_t *handle)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_RETURN, 0x0001);
}

esp_err_t gmirv2401_stb_get_code(gmirv2401_t *handle, uint16_t *code)
{
    return READ_REG(handle, GMIRV2401_REG_STB_CODE, code);
}

esp_err_t gmirv2401_stb_set_code(gmirv2401_t *handle, uint16_t code)
{
    return WRITE_REG(handle, GMIRV2401_REG_STB_CODE, code);
}

/* ================================================================
 * 红外学习
 * ================================================================ */

esp_err_t gmirv2401_ir_learn_start(gmirv2401_t *handle, uint8_t channel)
{
    if (channel < 1 || channel > GMIRV2401_IR_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t ch_val = (uint16_t)(channel - 1); /* 通道 0x0000-0x003F */
    ESP_LOGI(TAG, "IR learn start: channel %u (reg=0x%04X)", channel, ch_val);
    return WRITE_REG(handle, GMIRV2401_REG_IR_LEARN_START, ch_val);
}

esp_err_t gmirv2401_ir_learn_exit(gmirv2401_t *handle, uint8_t channel)
{
    if (channel < 1 || channel > GMIRV2401_IR_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t ch_val = (uint16_t)(channel - 1);
    return WRITE_REG(handle, GMIRV2401_REG_IR_LEARN_EXIT, ch_val);
}

esp_err_t gmirv2401_ir_learn_test(gmirv2401_t *handle, uint8_t channel)
{
    if (channel < 1 || channel > GMIRV2401_IR_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t ch_val = (uint16_t)(channel - 1);
    return WRITE_REG(handle, GMIRV2401_REG_IR_LEARN_TEST, ch_val);
}

esp_err_t gmirv2401_ir_read_waveform(gmirv2401_t *handle,
                                     uint8_t channel,
                                     uint8_t out_data[100])
{
    if (channel < 1 || channel > GMIRV2401_IR_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t reg_addr = GMIRV2401_IR_CHANNEL_REG(channel);
    return gmirv2401_read_registers(handle, reg_addr,
                                    GMIRV2401_REG_IR_CHANNEL_SIZE,
                                    out_data, 500);
}

esp_err_t gmirv2401_ir_write_waveform(gmirv2401_t *handle,
                                      uint8_t channel,
                                      const uint8_t data[100])
{
    if (channel < 1 || channel > GMIRV2401_IR_CHANNEL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t reg_addr = GMIRV2401_IR_CHANNEL_REG(channel);
    return gmirv2401_write_registers(handle, reg_addr,
                                     GMIRV2401_REG_IR_CHANNEL_SIZE,
                                     data, 500);
}

esp_err_t gmirv2401_ir_learn_poll_status(gmirv2401_t *handle,
                                         uint16_t *out_status)
{
    return READ_REG(handle, GMIRV2401_REG_IR_LEARN_START, out_status);
}

/* ================================================================
 * PLC 组态 API
 * ================================================================ */

esp_err_t gmirv2401_plc_ac_set_power(gmirv2401_t *handle, uint16_t power)
{ return WRITE_REG(handle, GMIRV2401_REG_PLC_AC_POWER, power); }
esp_err_t gmirv2401_plc_ac_get_power(gmirv2401_t *handle, uint16_t *power)
{ return READ_REG(handle, GMIRV2401_REG_PLC_AC_POWER, power); }

esp_err_t gmirv2401_plc_ac_set_temp(gmirv2401_t *handle, uint16_t temp)
{
    if (temp < 16 || temp > 31) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_PLC_AC_TEMP, temp);
}
esp_err_t gmirv2401_plc_ac_get_temp(gmirv2401_t *handle, uint16_t *temp)
{ return READ_REG(handle, GMIRV2401_REG_PLC_AC_TEMP, temp); }

esp_err_t gmirv2401_plc_ac_set_mode(gmirv2401_t *handle, uint16_t mode)
{
    if (mode > 4) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_PLC_AC_MODE, mode);
}
esp_err_t gmirv2401_plc_ac_get_mode(gmirv2401_t *handle, uint16_t *mode)
{ return READ_REG(handle, GMIRV2401_REG_PLC_AC_MODE, mode); }

esp_err_t gmirv2401_plc_ac_set_fan(gmirv2401_t *handle, uint16_t fan)
{
    if (fan > 3) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_PLC_AC_FAN, fan);
}
esp_err_t gmirv2401_plc_ac_get_fan(gmirv2401_t *handle, uint16_t *fan)
{ return READ_REG(handle, GMIRV2401_REG_PLC_AC_FAN, fan); }

esp_err_t gmirv2401_plc_ac_set_light(gmirv2401_t *handle, uint16_t light)
{
    if (light > 1) return ESP_ERR_INVALID_ARG;
    return WRITE_REG(handle, GMIRV2401_REG_PLC_AC_LIGHT, light);
}
esp_err_t gmirv2401_plc_ac_get_light(gmirv2401_t *handle, uint16_t *light)
{ return READ_REG(handle, GMIRV2401_REG_PLC_AC_LIGHT, light); }

esp_err_t gmirv2401_plc_ac_send(gmirv2401_t *handle, uint16_t code)
{
    return WRITE_REG(handle, GMIRV2401_REG_PLC_AC_CTRL_CODE, code);
}

esp_err_t gmirv2401_plc_ac_get_local_code(gmirv2401_t *handle, uint16_t *code)
{
    return READ_REG(handle, GMIRV2401_REG_PLC_AC_LOCAL_CODE, code);
}

esp_err_t gmirv2401_plc_ac_set_local_code(gmirv2401_t *handle, uint16_t code)
{
    return WRITE_REG(handle, GMIRV2401_REG_PLC_AC_LOCAL_CODE, code);
}

/* ================================================================
 * 通用按键发码
 * ================================================================ */

esp_err_t gmirv2401_send_key(gmirv2401_t *handle,
                             const gmirv2401_key_send_t *key)
{
    if (!handle || !key) return ESP_ERR_INVALID_ARG;

    uint8_t data[6];

    /* 002AH: 代码号 */
    data[0] = (uint8_t)(key->code >> 8);
    data[1] = (uint8_t)(key->code & 0xFF);

    /* 002BH: 状态字节1(默认开机+自动风速+灯光开+26℃) + 状态字节2(制冷+自动风速) */
    data[2] = 0xAA;  /* 开机, 自动风向, 灯光开, 26℃ */
    data[3] = 0x10;  /* 制冷模式, 自动风速 */

    /* 002CH: 状态字节3(键值) + 状态字节4(单一按键发送 + 指定码库) */
    data[4] = key->key_id;
    data[5] = 0x03;  /* 单一按键 + 使用指定码库 */

    ESP_LOGD(TAG, "send_key: code=%04X key=0x%02X",
             key->code, key->key_id);

    return gmirv2401_write_registers(handle, GMIRV2401_REG_AC_COMBO_CODE,
                                     3, data, 0);
}

/* ================================================================
 * 蓝牙透传
 * ================================================================ */

esp_err_t gmirv2401_ble_send(gmirv2401_t *handle,
                             const uint8_t *data, uint8_t data_len)
{
    if (!handle || !data || data_len == 0) return ESP_ERR_INVALID_ARG;

    /* 格式: [长度 2B(高位在前)] [数据 N bytes] */
    /* 寄存器 3000H 起: 长度占 1 个寄存器(2B), 数据占 ceil(data_len/2) 个 */
    uint16_t reg_count = 1 + (data_len + 1) / 2;
    uint16_t buf_size = reg_count * 2;
    uint8_t *tx = calloc(1, buf_size);
    if (!tx) return ESP_ERR_NO_MEM;

    tx[0] = 0x00;
    tx[1] = data_len;  /* 长度 (高 8 位在前, 所以 0x00xx) */
    memcpy(&tx[2], data, data_len);

    esp_err_t err = gmirv2401_write_registers(handle,
                                              GMIRV2401_REG_BLE_PASSTHROUGH,
                                              reg_count, tx, 500);
    free(tx);
    return err;
}
