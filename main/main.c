/**
 * @file    main.c
 * @brief   GMIRV2401 驱动使用示例
 *
 * 硬件连接:
 *   ESP32 TX (GPIO17) -> GMIRV2401 RX
 *   ESP32 RX (GPIO16) -> GMIRV2401 TX
 *
 * 演示:
 *   1. 初始化驱动
 *   2. 读取固件版本/芯片ID/MAC
 *   3. 空调控制 (读状态 / 设状态 / 组合控制)
 *   4. 电视控制 (电源 / 音量 / 频道)
 *   5. 机顶盒控制
 *   6. 红外学习 / PLC / BLE 透传 示例
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "gmirv2401.h"
#include "gmirv2401_ctrl.h"
#include "gmirv2401_reg.h"

static const char *TAG = "DEMO";

/* ---- UART 配置 (根据实际硬件修改) ---- */
#define UART_NUM       UART_NUM_1
#define TX_GPIO        17
#define RX_GPIO        16
#define DEV_ADDR       0xA5      /* 默认 165 */

static gmirv2401_t *g_dev = NULL;

/* ================================================================
 * 初始化
 * ================================================================ */

static void demo_init(void)
{
    gmirv2401_uart_config_t uart_cfg = {
        .uart_num    = UART_NUM,
        .tx_io       = TX_GPIO,
        .rx_io       = RX_GPIO,
        .rts_io      = UART_PIN_NO_CHANGE,
        .cts_io      = UART_PIN_NO_CHANGE,
        .baud_rate   = 9600,
        .rx_buf_size = 1024,
        .tx_buf_size = 512,
    };

    esp_err_t err = gmirv2401_init(&uart_cfg, DEV_ADDR, &g_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init failed: %d", err);
        return;
    }
    ESP_LOGI(TAG, "GMIRV2401 driver initialized");
}

/* ================================================================
 * 设备信息
 * ================================================================ */

static void demo_device_info(void)
{
    uint16_t fw_ver = 0, chip_id = 0, dev_addr = 0, baud = 0;
    uint8_t mac[6];

    gmirv2401_get_fw_version(g_dev, &fw_ver);
    gmirv2401_get_chip_id(g_dev, &chip_id);
    gmirv2401_get_mac(g_dev, mac);
    gmirv2401_get_device_addr(g_dev, &dev_addr);
    gmirv2401_get_baudrate(g_dev, &baud);

    ESP_LOGI(TAG, "=== Device Info ===");
    ESP_LOGI(TAG, "FW Version : 0x%04X", fw_ver);
    ESP_LOGI(TAG, "Chip ID    : 0x%04X", chip_id);
    ESP_LOGI(TAG, "MAC        : %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Dev Addr   : %u", dev_addr);
    ESP_LOGI(TAG, "Baudrate   : %u", baud);
}

/* ================================================================
 * 空调控制演示
 * ================================================================ */

static void demo_ac_control(void)
{
    esp_err_t err;

    /* --- 单一状态控制 --- */
    ESP_LOGI(TAG, "--- AC Single State ---");

    err = gmirv2401_ac_set_power(g_dev, 1);       /* 开机 */
    ESP_LOGI(TAG, "set_power(ON): %s", err == ESP_OK ? "OK" : "FAIL");

    vTaskDelay(pdMS_TO_TICKS(200));

    err = gmirv2401_ac_set_temp(g_dev, 26);       /* 26℃ */
    ESP_LOGI(TAG, "set_temp(26): %s", err == ESP_OK ? "OK" : "FAIL");

    vTaskDelay(pdMS_TO_TICKS(200));

    err = gmirv2401_ac_set_mode(g_dev, GMIRV2401_AC_MODE_COOL);  /* 制冷 */
    ESP_LOGI(TAG, "set_mode(COOL): %s", err == ESP_OK ? "OK" : "FAIL");

    vTaskDelay(pdMS_TO_TICKS(200));

    err = gmirv2401_ac_set_fan(g_dev, GMIRV2401_AC_FAN_AUTO);    /* 自动风速 */
    ESP_LOGI(TAG, "set_fan(AUTO): %s", err == ESP_OK ? "OK" : "FAIL");

    /* --- 读取状态 --- */
    uint16_t power, temp, mode, fan, light;
    gmirv2401_ac_get_power(g_dev, &power);
    gmirv2401_ac_get_temp(g_dev, &temp);
    gmirv2401_ac_get_mode(g_dev, &mode);
    gmirv2401_ac_get_fan(g_dev, &fan);
    gmirv2401_ac_get_light(g_dev, &light);
    ESP_LOGI(TAG, "AC State: power=%u temp=%u mode=%u fan=%u light=%u",
             power, temp, mode, fan, light);

    /* --- 组合状态控制 --- */
    ESP_LOGI(TAG, "--- AC Combo State ---");

    gmirv2401_ac_state_t combo = {
        .power  = 1,
        .swing  = 0,    /* 自动风向 */
        .light  = 1,    /* 开灯 */
        .temp   = 24,
        .mode   = GMIRV2401_AC_MODE_COOL,
        .fan    = GMIRV2401_AC_FAN_LOW,
        .code   = 0xFFFF,         /* 使用默认码库 */
        .use_specified_code = false,
        .use_single_key     = false,
        .key_value          = 0,
    };
    err = gmirv2401_ac_set_combo(g_dev, &combo);
    ESP_LOGI(TAG, "set_combo(): %s", err == ESP_OK ? "OK" : "FAIL");
}

/* ================================================================
 * 电视控制演示
 * ================================================================ */

static void demo_tv_control(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "--- TV Control ---");

    err = gmirv2401_tv_power(g_dev);
    ESP_LOGI(TAG, "tv_power: %s", err == ESP_OK ? "OK" : "FAIL");

    vTaskDelay(pdMS_TO_TICKS(500));

    err = gmirv2401_tv_vol_up(g_dev);
    ESP_LOGI(TAG, "tv_vol_up: %s", err == ESP_OK ? "OK" : "FAIL");

    vTaskDelay(pdMS_TO_TICKS(300));

    err = gmirv2401_tv_ch_up(g_dev);
    ESP_LOGI(TAG, "tv_ch_up: %s", err == ESP_OK ? "OK" : "FAIL");
}

/* ================================================================
 * 机顶盒控制演示
 * ================================================================ */

static void demo_stb_control(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "--- STB Control ---");

    err = gmirv2401_stb_power(g_dev);
    ESP_LOGI(TAG, "stb_power: %s", err == ESP_OK ? "OK" : "FAIL");

    vTaskDelay(pdMS_TO_TICKS(500));

    err = gmirv2401_stb_vol_up(g_dev);
    ESP_LOGI(TAG, "stb_vol_up: %s", err == ESP_OK ? "OK" : "FAIL");
}

/* ================================================================
 * 红外学习演示
 * ================================================================ */

static void demo_ir_learn(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "--- IR Learning ---");

    /* 启动通道 0 学习 */
    err = gmirv2401_ir_learn_start(g_dev, 0);
    ESP_LOGI(TAG, "ir_learn_start(ch1): %s", err == ESP_OK ? "OK" : "FAIL");

    /* 轮询等待学习完成 (30秒超时) */
    ESP_LOGI(TAG, "Waiting for IR learn to complete...");
    for (int i = 0; i < 60; i++) {
        vTaskDelay(pdMS_TO_TICKS(500));
        uint16_t status;
        if (gmirv2401_ir_learn_poll_status(g_dev, &status) == ESP_OK) {
            if (status == GMIRV2401_IR_LEARN_DONE) {
                ESP_LOGI(TAG, "IR learn done!");
                break;
            } else if (status == GMIRV2401_IR_LEARN_TIMEOUT) {
                ESP_LOGW(TAG, "IR learn timeout");
                break;
            }
        }
    }

    /* 读取学习到的波形 */
    uint8_t waveform[100];
    err = gmirv2401_ir_read_waveform(g_dev, 0, waveform);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Waveform read OK (100 bytes)");
    }

    /* 测试发送 */
    err = gmirv2401_ir_learn_test(g_dev, 0);
    ESP_LOGI(TAG, "ir_learn_test(ch1): %s", err == ESP_OK ? "OK" : "FAIL");
}

/* ================================================================
 * 通用按键发码演示
 * ================================================================ */

static void demo_send_key(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "--- Send Generic Key ---");

    gmirv2401_key_send_t key = {
        .dev_type = GMIRV2401_DEV_TYPE_TV,
        .code     = 0x1234,           /* 假设的电视码库代号 */
        .key_id   = GMIRV2401_KEY_MUTE, /* 静音键 */
    };

    err = gmirv2401_send_key(g_dev, &key);
    ESP_LOGI(TAG, "send_key(mute): %s", err == ESP_OK ? "OK" : "FAIL");
}

/* ================================================================
 * 蓝牙透传演示
 * ================================================================ */

static void demo_ble_passthrough(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "--- BLE Passthrough ---");

    const char *msg = "Hello BLE";
    err = gmirv2401_ble_send(g_dev, (const uint8_t *)msg,
                             (uint8_t)strlen(msg));
    ESP_LOGI(TAG, "ble_send: %s", err == ESP_OK ? "OK" : "FAIL");
}

/* ================================================================
 * PLC 演示
 * ================================================================ */

static void demo_plc(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "--- PLC Control ---");

    uint16_t local_code;
    err = gmirv2401_plc_ac_get_local_code(g_dev, &local_code);
    ESP_LOGI(TAG, "local_code: 0x%04X (err=%d)", local_code, err);

    /* 使用 0xFFFF 发送默认代号 */
    err = gmirv2401_plc_ac_send(g_dev, 0xFFFF);
    ESP_LOGI(TAG, "plc_ac_send(0xFFFF): %s", err == ESP_OK ? "OK" : "FAIL");
}

/* ================================================================
 * 主入口
 * ================================================================ */

void app_main(void)
{
    ESP_LOGI(TAG, "=== GMIRV2401 Driver Demo ===");

    demo_init();
    if (!g_dev) {
        ESP_LOGE(TAG, "Driver init failed, abort");
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    /* 1. 设备信息 */
    demo_device_info();

    /* 2. 空调控制 */
    demo_ac_control();

    /* 3. 电视控制 */
    demo_tv_control();

    /* 4. 机顶盒控制 */
    demo_stb_control();

    /* 5. 通用按键发码 */
    demo_send_key();

    /* 6. PLC */
    demo_plc();

    /* 7. 红外学习 (需要实际红外遥控器对准芯片) */
    /* demo_ir_learn(); */

    /* 8. 蓝牙透传 */
    /* demo_ble_passthrough(); */

    ESP_LOGI(TAG, "=== Demo Complete ===");

    /* 保持运行 */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

