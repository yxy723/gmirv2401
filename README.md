# GMIRV2401 ESP-IDF Driver

基于 Modbus-RTU 协议的 GMIRV2401 万能红外转发芯片 ESP-IDF 驱动，支持空调/电视/机顶盒码库控制、64 通道红外学习和蓝牙透传。

## 硬件

| 芯片 | 接口 | 协议 |
|------|------|------|
| GMIRV2401 (G&M TECH) | UART 9600-8-N-1 | Modbus-RTU (03H/06H/10H) |

**接线示例 (ESP32):**

```
ESP32 TX (GPIO17)  →  GMIRV2401 RX
ESP32 RX (GPIO16)  →  GMIRV2401 TX
```

## 功能覆盖

| 模块 | 功能 |
|------|------|
| **核心** | Modbus-RTU 帧构建/解析、CRC16 查表、UART 收发、互斥锁保护 |
| **设备信息** | 固件版本、芯片ID、MAC 地址、设备地址、波特率读写、芯片复位 |
| **空调** | 单一状态控制 (电源/温度/模式/风速/灯光)、组合状态 6 字节控制、一键匹配 |
| **电视** | 电源/音量/频道/信号源/返回、码库代号读写 |
| **机顶盒** | 电源/音量/频道/信号源/返回、码库代号读写 |
| **红外学习** | 启动/退出/测试、100 字节波形读写 (64 通道\)、状态轮询 |
| **PLC 组态** | 0040H-0046H 寄存器读写、发码触发 |
| **蓝牙透传** | 3000H 寄存器数据透传 |
| **通用按键** | 任意设备按键发码 (通过组合状态寄存器) |

## 项目结构

```
gmirv2401/
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   └── main.c                          # 使用示例
└── components/
    └── gmirv2401/
        ├── CMakeLists.txt
        ├── include/
        │   ├── gmirv2401.h             # 核心驱动 API
        │   ├── gmirv2401_ctrl.h        # 设备控制 API (AC/TV/STB/IR/BLE)
        │   └── gmirv2401_reg.h         # 寄存器地址宏定义
        └── src/
            ├── gmirv2401.c             # 核心实现 (UART+Modbus+CRC16)
            └── gmirv2401_ctrl.c        # 设备控制实现
```

## 快速开始

### 环境要求

- ESP-IDF v5.x (已在 v5.4.2 验证)
- ESP32 / ESP32-S3 / ESP32-C3 等
- VS Code + ESP-IDF 插件 (推荐)

### 编译 & 烧录

```bash
# 设置目标芯片
idf.py set-target esp32

# 编译
idf.py build

# 烧录
idf.py -p COMx flash monitor
```

## API 示例

### 初始化

```c
#include "gmirv2401.h"

gmirv2401_uart_config_t uart_cfg = {
    .uart_num = UART_NUM_1,
    .tx_io    = 17,
    .rx_io    = 16,
    .rts_io   = UART_PIN_NO_CHANGE,
    .cts_io   = UART_PIN_NO_CHANGE,
    .baud_rate = 9600,
};

gmirv2401_t *dev = NULL;
gmirv2401_init(&uart_cfg, 0xA5, &dev);
```

### 空调控制

```c
#include "gmirv2401_ctrl.h"

// 单一状态
gmirv2401_ac_set_power(dev, 1);
gmirv2401_ac_set_temp(dev, 26);
gmirv2401_ac_set_mode(dev, GMIRV2401_AC_MODE_COOL);

// 组合状态 (一次性设置所有参数)
gmirv2401_ac_state_t combo = {
    .power = 1, .temp = 24,
    .mode = GMIRV2401_AC_MODE_COOL,
    .fan  = GMIRV2401_AC_FAN_AUTO,
    .code = 0xFFFF,
};
gmirv2401_ac_set_combo(dev, &combo);
```

### 电视/机顶盒

```c
gmirv2401_tv_power(dev);
gmirv2401_tv_vol_up(dev);
gmirv2401_stb_ch_up(dev);
```

### 红外学习

```c
gmirv2401_ir_learn_start(dev, 0);  // 启动通道0学习 (地址 0x5000)

// 轮询状态
uint16_t status;
gmirv2401_ir_learn_poll_status(dev, &status);
if (status == 0x8002) { /* 学习完成 */ }

// 读取/写入波形
uint8_t waveform[100];
gmirv2401_ir_read_waveform(dev, 0, waveform);
gmirv2401_ir_learn_test(dev, 0);   // 测试发送
```

### 蓝牙透传

```c
gmirv2401_ble_send(dev, (uint8_t*)"Hello", 5);
```

## 参考文档

- [GMIRV240x-通信协议_V1.8.pdf](https://github.com/yxy723/gmirv2401) — 完整协议规范

## License

MIT

