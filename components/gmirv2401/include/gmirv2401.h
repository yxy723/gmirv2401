/**
 * @file    gmirv2401.h
 * @brief   GMIRV2401 红外转发芯片驱动 - 公共头文件
 *
 * 基于 Modbus-RTU 协议，UART 9600-8-N-1
 * 支持 BLE 透传
 */

#ifndef GMIRV2401_H
#define GMIRV2401_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * 配置结构体
 * ================================================================ */

/** UART 端口配置 */
typedef struct {
    uart_port_t uart_num;       /**< UART 端口号 (UART_NUM_0/1/2) */
    int tx_io;                  /**< TX GPIO */
    int rx_io;                  /**< RX GPIO */
    int rts_io;                 /**< RTS GPIO (UART_PIN_NO_CHANGE 表示不使用) */
    int cts_io;                 /**< CTS GPIO */
    int baud_rate;              /**< 默认 9600 */
    int rx_buf_size;            /**< 接收缓冲区大小 */
    int tx_buf_size;            /**< 发送缓冲区大小 */
} gmirv2401_uart_config_t;

/** Modbus 响应回调类型 */
typedef void (*gmirv2401_response_cb_t)(uint8_t func_code, const uint8_t *data,
                                        uint16_t data_len, esp_err_t err,
                                        void *user_ctx);

/** 驱动句柄 (opaque) */
typedef struct gmirv2401 gmirv2401_t;

/* ================================================================
 * 生命周期 API
 * ================================================================ */

/**
 * @brief 初始化 GMIRV2401 驱动
 *
 * @param[in]  uart_cfg  UART 配置
 * @param[in]  dev_addr  芯片 Modbus 设备地址 (1-255, 需与芯片当前地址一致)
 * @param[out] out_handle 返回的驱动句柄
 * @return ESP_OK 成功
 */
esp_err_t gmirv2401_init(const gmirv2401_uart_config_t *uart_cfg,
                         uint8_t dev_addr,
                         gmirv2401_t **out_handle);

/**
 * @brief 反初始化，释放资源
 */
esp_err_t gmirv2401_deinit(gmirv2401_t *handle);

/* ================================================================
 * 底层 Modbus 命令 API
 * ================================================================ */

/**
 * @brief 读取多个寄存器 (功能码 03H)
 *
 * @param reg_addr  起始寄存器地址
 * @param reg_count 寄存器数量
 * @param out_data  输出数据缓冲 (调用者分配, 长度 ≥ reg_count * 2)
 * @param timeout_ms 超时时间
 * @return ESP_OK 成功
 */
esp_err_t gmirv2401_read_registers(gmirv2401_t *handle,
                                   uint16_t reg_addr, uint16_t reg_count,
                                   uint8_t *out_data, uint32_t timeout_ms);

/**
 * @brief 写单个寄存器 (功能码 06H)
 *
 * @param reg_addr  寄存器地址
 * @param value     写入值
 * @param timeout_ms 超时时间
 * @return ESP_OK 成功
 */
esp_err_t gmirv2401_write_register(gmirv2401_t *handle,
                                   uint16_t reg_addr, uint16_t value,
                                   uint32_t timeout_ms);

/**
 * @brief 写多个寄存器 (功能码 10H)
 *
 * @param reg_addr  起始寄存器地址
 * @param reg_count 寄存器数量
 * @param data      数据缓冲 (每个寄存器 2 字节, 高位在前)
 * @param timeout_ms 超时时间
 * @return ESP_OK 成功
 */
esp_err_t gmirv2401_write_registers(gmirv2401_t *handle,
                                    uint16_t reg_addr, uint16_t reg_count,
                                    const uint8_t *data, uint32_t timeout_ms);

/* ================================================================
 * 便捷读取 API
 * ================================================================ */

/** 读取单个 uint16 寄存器值 */
esp_err_t gmirv2401_read_u16(gmirv2401_t *handle, uint16_t reg_addr,
                             uint16_t *out_value, uint32_t timeout_ms);

/** 读取单个 uint32 寄存器值 (2 个连续寄存器) */
esp_err_t gmirv2401_read_u32(gmirv2401_t *handle, uint16_t reg_addr,
                             uint32_t *out_value, uint32_t timeout_ms);

/* ================================================================
 * 设备信息 API
 * ================================================================ */

esp_err_t gmirv2401_get_fw_version(gmirv2401_t *handle, uint16_t *ver);
esp_err_t gmirv2401_get_chip_id(gmirv2401_t *handle, uint16_t *id);
esp_err_t gmirv2401_get_mac(gmirv2401_t *handle, uint8_t mac[6]);
esp_err_t gmirv2401_get_device_addr(gmirv2401_t *handle, uint16_t *addr);
esp_err_t gmirv2401_set_device_addr(gmirv2401_t *handle, uint16_t addr);
esp_err_t gmirv2401_get_baudrate(gmirv2401_t *handle, uint16_t *baud);
esp_err_t gmirv2401_set_baudrate(gmirv2401_t *handle, uint16_t baud);
esp_err_t gmirv2401_reset_chip(gmirv2401_t *handle);

/* ================================================================
 * 互感器 / ADC API
 * ================================================================ */

esp_err_t gmirv2401_get_power_status(gmirv2401_t *handle, uint16_t *status);
esp_err_t gmirv2401_get_adc_value(gmirv2401_t *handle, uint16_t *mv);

/* ================================================================
 * CRC16 工具函数 (也可对外使用)
 * ================================================================ */

uint16_t gmirv2401_crc16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* GMIRV2401_H */
