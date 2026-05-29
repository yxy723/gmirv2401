/**
 * @file    gmirv2401.c
 * @brief   GMIRV2401 红外转发芯片驱动核心实现
 *
 * UART + Modbus-RTU 协议栈 + CRC16
 */

#include "gmirv2401.h"
#include "gmirv2401_reg.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

static const char *TAG = "GMIRV2401";

/* ================================================================
 * 内部结构
 * ================================================================ */

/** Modbus-RTU 功能码 */
#define MODBUS_FC_READ_MULTIPLE    0x03
#define MODBUS_FC_WRITE_SINGLE     0x06
#define MODBUS_FC_WRITE_MULTIPLE   0x10

/** Modbus 异常功能码偏移 */
#define MODBUS_FC_EXCEPTION_OFFSET 0x80

/** 帧头尾常量 (BLE) */
#define GMIRV2401_BLE_SOF           0xFF55
#define GMIRV2401_BLE_EOF           0xFFAA

/** 最大响应数据长度 */
#define GMIRV2401_MAX_DATA_LEN      256

/** 响应超时默认 */
#define GMIRV2401_DEFAULT_TIMEOUT_MS 500

/** Modbus-RTU 帧间隔 (3.5 字符时间 ≈ 参数默认) */
#define GMIRV2401_FRAME_GAP_US      5000

struct gmirv2401 {
    uart_port_t   uart_num;
    uint8_t       dev_addr;        /**< Modbus 设备地址 */
    int           baud_rate;
    SemaphoreHandle_t tx_mutex;    /**< 发送互斥锁 */
};

/* ================================================================
 * CRC16-Modbus 查表实现
 * ================================================================ */

static const uint16_t s_crc16_table[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

uint16_t gmirv2401_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc = (crc >> 8) ^ s_crc16_table[(crc ^ *data++) & 0xFF];
    }
    return crc;
}

/* ================================================================
 * 帧构建 / 解析
 * ================================================================ */

/**
 * 构建 Modbus-RTU 读寄存器命令帧
 * 返回帧长度，0 表示失败
 */
static uint16_t build_read_frame(uint8_t *buf, uint16_t buf_size,
                                 uint8_t addr, uint16_t reg, uint16_t count)
{
    if (buf_size < 8) return 0;
    buf[0] = addr;
    buf[1] = MODBUS_FC_READ_MULTIPLE;
    buf[2] = (uint8_t)(reg >> 8);
    buf[3] = (uint8_t)(reg & 0xFF);
    buf[4] = (uint8_t)(count >> 8);
    buf[5] = (uint8_t)(count & 0xFF);
    uint16_t crc = gmirv2401_crc16(buf, 6);
    buf[6] = (uint8_t)(crc & 0xFF);
    buf[7] = (uint8_t)(crc >> 8);
    return 8;
}

/**
 * 构建 Modbus-RTU 写单个寄存器命令帧
 */
static uint16_t build_write_single_frame(uint8_t *buf, uint16_t buf_size,
                                         uint8_t addr, uint16_t reg, uint16_t val)
{
    if (buf_size < 8) return 0;
    buf[0] = addr;
    buf[1] = MODBUS_FC_WRITE_SINGLE;
    buf[2] = (uint8_t)(reg >> 8);
    buf[3] = (uint8_t)(reg & 0xFF);
    buf[4] = (uint8_t)(val >> 8);
    buf[5] = (uint8_t)(val & 0xFF);
    uint16_t crc = gmirv2401_crc16(buf, 6);
    buf[6] = (uint8_t)(crc & 0xFF);
    buf[7] = (uint8_t)(crc >> 8);
    return 8;
}

/**
 * 构建 Modbus-RTU 写多个寄存器命令帧
 * data 中每 2 字节一个寄存器，高位在前
 */
static uint16_t build_write_multi_frame(uint8_t *buf, uint16_t buf_size,
                                        uint8_t addr, uint16_t reg,
                                        uint16_t count, const uint8_t *data)
{
    uint16_t byte_count = count * 2;
    uint16_t total = 9 + byte_count;
    if (buf_size < total) return 0;

    buf[0] = addr;
    buf[1] = MODBUS_FC_WRITE_MULTIPLE;
    buf[2] = (uint8_t)(reg >> 8);
    buf[3] = (uint8_t)(reg & 0xFF);
    buf[4] = (uint8_t)(count >> 8);
    buf[5] = (uint8_t)(count & 0xFF);
    buf[6] = (uint8_t)byte_count;
    memcpy(&buf[7], data, byte_count);
    uint16_t crc = gmirv2401_crc16(buf, 7 + byte_count);
    buf[7 + byte_count]     = (uint8_t)(crc & 0xFF);
    buf[7 + byte_count + 1] = (uint8_t)(crc >> 8);
    return total;
}

/* ================================================================
 * 发送 / 接收
 * ================================================================ */

static esp_err_t uart_send(gmirv2401_t *dev, const uint8_t *data, uint16_t len)
{
    int sent = uart_write_bytes(dev->uart_num, data, len);
    if (sent < 0) {
        ESP_LOGE(TAG, "uart_write_bytes failed");
        return ESP_FAIL;
    }
    uart_wait_tx_done(dev->uart_num, pdMS_TO_TICKS(100));
    return ESP_OK;
}

static esp_err_t uart_recv(gmirv2401_t *dev, uint8_t *buf, uint16_t buf_size,
                           uint16_t *out_len, uint32_t timeout_ms)
{
    uint16_t total = 0;
    TickType_t start = xTaskGetTickCount();
    TickType_t timeout = pdMS_TO_TICKS(timeout_ms);

    while (total < buf_size) {
        int len = uart_read_bytes(dev->uart_num, &buf[total],
                                  buf_size - total,
                                  pdMS_TO_TICKS(100));
        if (len > 0) {
            total += (uint16_t)len;
            /* 短暂等待看是否有更多数据 */
            vTaskDelay(pdMS_TO_TICKS(10));
            int extra = uart_read_bytes(dev->uart_num, &buf[total],
                                        buf_size - total,
                                        pdMS_TO_TICKS(50));
            if (extra > 0) total += (uint16_t)extra;
            break;
        }

        if ((xTaskGetTickCount() - start) >= timeout) {
            *out_len = 0;
            return ESP_ERR_TIMEOUT;
        }
    }

    *out_len = total;
    return (total > 0) ? ESP_OK : ESP_ERR_TIMEOUT;
}

/**
 * 根据功能码验证响应帧长度
 */
static bool validate_response_len(uint8_t func, uint16_t total_len,
                                  uint16_t expected_data_regs,
                                  uint16_t *out_data_offset)
{
    switch (func) {
    case MODBUS_FC_READ_MULTIPLE: {
        /* 地址(1) + 功能码(1) + 字节数(1) + 数据 + CRC(2) */
        uint16_t byte_count = (uint16_t)(expected_data_regs * 2);
        if (total_len != (uint16_t)(3 + byte_count + 2)) return false;
        *out_data_offset = 3;
        return true;
    }
    case MODBUS_FC_WRITE_SINGLE:
    case MODBUS_FC_WRITE_MULTIPLE: {
        /* 地址(1) + 功能码(1) + 寄存器地址(2) + 数据/数量(2) + CRC(2) = 8 */
        if (total_len != 8) return false;
        *out_data_offset = 0;
        return true;
    }
    default:
        return false;
    }
}

/* ================================================================
 * 核心事务
 * ================================================================ */

static esp_err_t modbus_transaction(gmirv2401_t *dev,
                                    const uint8_t *tx_buf, uint16_t tx_len,
                                    uint8_t expected_func,
                                    uint16_t expected_data_regs,
                                    uint8_t *out_data_buf, uint16_t out_buf_size,
                                    uint16_t *out_data_len,
                                    uint32_t timeout_ms)
{
    /* 清除接收缓冲 */
    uart_flush_input(dev->uart_num);

    /* 发送命令 */
    esp_err_t err = uart_send(dev, tx_buf, tx_len);
    if (err != ESP_OK) return err;

    /* 接收响应 */
    uint8_t rx_buf[GMIRV2401_MAX_DATA_LEN];
    uint16_t rx_len = 0;
    err = uart_recv(dev, rx_buf, sizeof(rx_buf), &rx_len, timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "recv timeout");
        return err;
    }

    /* 验证 CRC */
    if (rx_len < 4) {
        ESP_LOGW(TAG, "rx too short: %u", rx_len);
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint16_t recv_crc = ((uint16_t)rx_buf[rx_len - 1] << 8) | rx_buf[rx_len - 2];
    uint16_t calc_crc = gmirv2401_crc16(rx_buf, rx_len - 2);
    if (recv_crc != calc_crc) {
        ESP_LOGW(TAG, "CRC mismatch: recv=0x%04X calc=0x%04X", recv_crc, calc_crc);
        return ESP_ERR_INVALID_CRC;
    }

    /* 验证地址 */
    if (rx_buf[0] != dev->dev_addr && rx_buf[0] != 0x00 && rx_buf[0] != 0xFF) {
        ESP_LOGW(TAG, "addr mismatch: expected %02X got %02X", dev->dev_addr, rx_buf[0]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint8_t func = rx_buf[1];

    /* 检查异常响应 */
    if (func & MODBUS_FC_EXCEPTION_OFFSET) {
        uint8_t exception_code = (rx_len >= 3) ? rx_buf[2] : 0;
        ESP_LOGW(TAG, "Modbus exception: func=%02X code=%02X", func, exception_code);
        return ESP_FAIL;
    }

    /* 验证功能码 */
    if (func != expected_func) {
        ESP_LOGW(TAG, "func mismatch: expected %02X got %02X", expected_func, func);
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* 验证响应长度并提取数据 */
    uint16_t data_offset = 0;
    if (!validate_response_len(func, rx_len, expected_data_regs, &data_offset)) {
        ESP_LOGW(TAG, "invalid response length: %u for func %02X", rx_len, func);
        return ESP_ERR_INVALID_SIZE;
    }

    /* 提取数据 */
    if (out_data_buf && data_offset > 0) {
        uint16_t copy_len = rx_len - data_offset - 2; /* 减去 CRC */
        if (copy_len > out_buf_size) copy_len = out_buf_size;
        memcpy(out_data_buf, &rx_buf[data_offset], copy_len);
        if (out_data_len) *out_data_len = copy_len;
    } else if (out_data_len) {
        *out_data_len = 0;
    }

    return ESP_OK;
}

/* ================================================================
 * 生命周期
 * ================================================================ */

esp_err_t gmirv2401_init(const gmirv2401_uart_config_t *uart_cfg,
                         uint8_t dev_addr,
                         gmirv2401_t **out_handle)
{
    if (!uart_cfg || !out_handle) return ESP_ERR_INVALID_ARG;

    gmirv2401_t *dev = calloc(1, sizeof(gmirv2401_t));
    if (!dev) return ESP_ERR_NO_MEM;

    dev->uart_num = uart_cfg->uart_num;
    dev->dev_addr = dev_addr;
    dev->baud_rate = uart_cfg->baud_rate ? uart_cfg->baud_rate : 9600;

    /* UART 配置 */
    uart_config_t uart_conf = {
        .baud_rate  = dev->baud_rate,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_param_config(dev->uart_num, &uart_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config: %d", err);
        goto fail;
    }

    err = uart_set_pin(dev->uart_num, uart_cfg->tx_io, uart_cfg->rx_io,
                       uart_cfg->rts_io, uart_cfg->cts_io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin: %d", err);
        goto fail;
    }

    int rx_buf = uart_cfg->rx_buf_size ? uart_cfg->rx_buf_size : 512;
    int tx_buf = uart_cfg->tx_buf_size ? uart_cfg->tx_buf_size : 256;
    err = uart_driver_install(dev->uart_num, rx_buf, tx_buf, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install: %d", err);
        goto fail;
    }

    dev->tx_mutex = xSemaphoreCreateMutex();
    if (!dev->tx_mutex) {
        err = ESP_ERR_NO_MEM;
        goto fail_uart;
    }

    ESP_LOGI(TAG, "init OK: uart=%d addr=%d baud=%d",
             dev->uart_num, dev->dev_addr, dev->baud_rate);

    *out_handle = dev;
    return ESP_OK;

fail_uart:
    uart_driver_delete(dev->uart_num);
fail:
    free(dev);
    return err;
}

esp_err_t gmirv2401_deinit(gmirv2401_t *handle)
{
    if (!handle) return ESP_ERR_INVALID_ARG;

    if (handle->tx_mutex) {
        vSemaphoreDelete(handle->tx_mutex);
    }
    uart_driver_delete(handle->uart_num);
    free(handle);
    return ESP_OK;
}

/* ================================================================
 * Modbus 命令 API
 * ================================================================ */

esp_err_t gmirv2401_read_registers(gmirv2401_t *handle,
                                   uint16_t reg_addr, uint16_t reg_count,
                                   uint8_t *out_data, uint32_t timeout_ms)
{
    if (!handle || !out_data || reg_count == 0) return ESP_ERR_INVALID_ARG;
    if (timeout_ms == 0) timeout_ms = GMIRV2401_DEFAULT_TIMEOUT_MS;

    xSemaphoreTake(handle->tx_mutex, portMAX_DELAY);

    uint8_t tx_buf[8];
    uint16_t tx_len = build_read_frame(tx_buf, sizeof(tx_buf),
                                       handle->dev_addr, reg_addr, reg_count);
    if (tx_len == 0) {
        xSemaphoreGive(handle->tx_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    uint16_t data_len = 0;
    esp_err_t err = modbus_transaction(handle, tx_buf, tx_len,
                                       MODBUS_FC_READ_MULTIPLE,
                                       reg_count,
                                       out_data, reg_count * 2,
                                       &data_len, timeout_ms);

    xSemaphoreGive(handle->tx_mutex);
    return err;
}

esp_err_t gmirv2401_write_register(gmirv2401_t *handle,
                                   uint16_t reg_addr, uint16_t value,
                                   uint32_t timeout_ms)
{
    if (!handle) return ESP_ERR_INVALID_ARG;
    if (timeout_ms == 0) timeout_ms = GMIRV2401_DEFAULT_TIMEOUT_MS;

    xSemaphoreTake(handle->tx_mutex, portMAX_DELAY);

    uint8_t tx_buf[8];
    uint16_t tx_len = build_write_single_frame(tx_buf, sizeof(tx_buf),
                                               handle->dev_addr,
                                               reg_addr, value);
    if (tx_len == 0) {
        xSemaphoreGive(handle->tx_mutex);
        return ESP_ERR_INVALID_SIZE;
    }

    esp_err_t err = modbus_transaction(handle, tx_buf, tx_len,
                                       MODBUS_FC_WRITE_SINGLE,
                                       1,
                                       NULL, 0, NULL, timeout_ms);

    xSemaphoreGive(handle->tx_mutex);
    return err;
}

esp_err_t gmirv2401_write_registers(gmirv2401_t *handle,
                                    uint16_t reg_addr, uint16_t reg_count,
                                    const uint8_t *data, uint32_t timeout_ms)
{
    if (!handle || !data || reg_count == 0) return ESP_ERR_INVALID_ARG;
    if (timeout_ms == 0) timeout_ms = GMIRV2401_DEFAULT_TIMEOUT_MS;

    xSemaphoreTake(handle->tx_mutex, portMAX_DELAY);

    uint16_t byte_count = reg_count * 2;
    uint16_t buf_size = 9 + byte_count;
    uint8_t *tx_buf = malloc(buf_size);
    if (!tx_buf) {
        xSemaphoreGive(handle->tx_mutex);
        return ESP_ERR_NO_MEM;
    }

    uint16_t tx_len = build_write_multi_frame(tx_buf, buf_size,
                                              handle->dev_addr,
                                              reg_addr, reg_count, data);
    esp_err_t err;
    if (tx_len == 0) {
        err = ESP_ERR_INVALID_SIZE;
    } else {
        err = modbus_transaction(handle, tx_buf, tx_len,
                                 MODBUS_FC_WRITE_MULTIPLE,
                                 reg_count,
                                 NULL, 0, NULL, timeout_ms);
    }

    free(tx_buf);
    xSemaphoreGive(handle->tx_mutex);
    return err;
}

/* ================================================================
 * 便捷读取 API
 * ================================================================ */

esp_err_t gmirv2401_read_u16(gmirv2401_t *handle, uint16_t reg_addr,
                             uint16_t *out_value, uint32_t timeout_ms)
{
    uint8_t data[2];
    esp_err_t err = gmirv2401_read_registers(handle, reg_addr, 1,
                                             data, timeout_ms);
    if (err == ESP_OK && out_value) {
        *out_value = ((uint16_t)data[0] << 8) | data[1];
    }
    return err;
}

esp_err_t gmirv2401_read_u32(gmirv2401_t *handle, uint16_t reg_addr,
                             uint32_t *out_value, uint32_t timeout_ms)
{
    uint8_t data[4];
    esp_err_t err = gmirv2401_read_registers(handle, reg_addr, 2,
                                             data, timeout_ms);
    if (err == ESP_OK && out_value) {
        *out_value = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                     ((uint32_t)data[2] << 8)  |  (uint32_t)data[3];
    }
    return err;
}

/* ================================================================
 * 设备信息 API
 * ================================================================ */

esp_err_t gmirv2401_get_fw_version(gmirv2401_t *handle, uint16_t *ver)
{
    return gmirv2401_read_u16(handle, GMIRV2401_REG_FW_VERSION, ver, 0);
}

esp_err_t gmirv2401_get_chip_id(gmirv2401_t *handle, uint16_t *id)
{
    return gmirv2401_read_u16(handle, GMIRV2401_REG_CHIP_ID, id, 0);
}

esp_err_t gmirv2401_get_mac(gmirv2401_t *handle, uint8_t mac[6])
{
    return gmirv2401_read_registers(handle, GMIRV2401_REG_MAC_ADDR, 3, mac, 0);
}

esp_err_t gmirv2401_get_device_addr(gmirv2401_t *handle, uint16_t *addr)
{
    return gmirv2401_read_u16(handle, GMIRV2401_REG_DEV_ADDR, addr, 0);
}

esp_err_t gmirv2401_set_device_addr(gmirv2401_t *handle, uint16_t addr)
{
    if (addr < 1 || addr > 255) return ESP_ERR_INVALID_ARG;
    esp_err_t err = gmirv2401_write_register(handle, GMIRV2401_REG_DEV_ADDR, addr, 0);
    if (err == ESP_OK) {
        handle->dev_addr = (uint8_t)addr;
    }
    return err;
}

esp_err_t gmirv2401_get_baudrate(gmirv2401_t *handle, uint16_t *baud)
{
    return gmirv2401_read_u16(handle, GMIRV2401_REG_BAUDRATE, baud, 0);
}

esp_err_t gmirv2401_set_baudrate(gmirv2401_t *handle, uint16_t baud)
{
    return gmirv2401_write_register(handle, GMIRV2401_REG_BAUDRATE, baud, 0);
}

esp_err_t gmirv2401_reset_chip(gmirv2401_t *handle)
{
    return gmirv2401_write_register(handle, GMIRV2401_REG_RESET, 0x0001, 0);
}

/* ================================================================
 * 互感器 / ADC API
 * ================================================================ */

esp_err_t gmirv2401_get_power_status(gmirv2401_t *handle, uint16_t *status)
{
    return gmirv2401_read_u16(handle, GMIRV2401_REG_CT_STATUS, status, 0);
}

esp_err_t gmirv2401_get_adc_value(gmirv2401_t *handle, uint16_t *mv)
{
    return gmirv2401_read_u16(handle, GMIRV2401_REG_ADC_VALUE, mv, 0);
}
