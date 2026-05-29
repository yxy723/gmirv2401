/**
 * @file    gmirv2401_reg.h
 * @brief   GMIRV2401 寄存器地址定义
 *
 * 基于 GMIRV240x-通信协议_V1.8
 * UART 9600-8-N-1, Modbus-RTU 协议
 */

#ifndef GMIRV2401_REG_H
#define GMIRV2401_REG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 * 只读寄存器
 * ================================================================ */

/** 固件版本号 (RO) */
#define GMIRV2401_REG_FW_VERSION        0x0000
/** 芯片 ID (RO) */
#define GMIRV2401_REG_CHIP_ID           0x0001
/** MAC 地址 (RO, 3 个寄存器: 6 bytes) */
#define GMIRV2401_REG_MAC_ADDR          0x0004

/* ================================================================
 * 设备配置寄存器
 * ================================================================ */

/** 设备地址 (1-255) */
#define GMIRV2401_REG_DEV_ADDR          0x0002
/** 串口波特率 0-20 */
#define GMIRV2401_REG_BAUDRATE          0x0003
/** 芯片复位 (写入 0x0001 复位) */
#define GMIRV2401_REG_RESET             0x0009

/* ================================================================
 * 一键匹配 / 红外学习寄存器
 * ================================================================ */

/** 空调一键匹配 (0001H=启动, 8003H=超时, XXXXH=匹配成功码库代号) */
#define GMIRV2401_REG_AC_MATCH          0x0010
/** 电视匹配 (暂不支持) */
#define GMIRV2401_REG_TV_MATCH          0x0011
/** 机顶盒匹配 (暂不支持) */
#define GMIRV2401_REG_STB_MATCH         0x0012
/** 风扇匹配 (暂不支持) */
#define GMIRV2401_REG_FAN_MATCH         0x0013
/** 投影仪匹配 (暂不支持) */
#define GMIRV2401_REG_PROJ_MATCH        0x0014
/** 其他设备匹配 (暂不支持) */
#define GMIRV2401_REG_OTHER_MATCH       0x0015

/** 启动红外学习 (通道 0x0000-0x003F) */
#define GMIRV2401_REG_IR_LEARN_START    0x0016
/** 退出红外学习 */
#define GMIRV2401_REG_IR_LEARN_EXIT     0x0017
/** 测试红外学习 */
#define GMIRV2401_REG_IR_LEARN_TEST     0x0018

/* ================================================================
 * 设备码库代号寄存器
 * ================================================================ */

/** 空调码库代号 */
#define GMIRV2401_REG_AC_CODE           0x001F
/** 电视码库代号 */
#define GMIRV2401_REG_TV_CODE           0x0020
/** 机顶盒码库代号 */
#define GMIRV2401_REG_STB_CODE          0x0021

/* ================================================================
 * 空调单一状态寄存器 (0025H-0029H)
 * ================================================================ */

/** 空调电源状态: 1-开, 0-关 */
#define GMIRV2401_REG_AC_POWER          0x0025
/** 空调温度状态: 0-E → 16-30℃ */
#define GMIRV2401_REG_AC_TEMP           0x0026
/** 空调模式: 0-自动, 1-制冷, 2-除湿, 3-送风, 4-制热 */
#define GMIRV2401_REG_AC_MODE           0x0027
/** 空调风速: 0-自动, 1-一档, 2-二档, 3-三档 */
#define GMIRV2401_REG_AC_FAN            0x0028
/** 空调灯光: 0-关灯, 1-开灯 */
#define GMIRV2401_REG_AC_LIGHT          0x0029

/* ================================================================
 * 空调组合状态寄存器 (002AH-002CH)
 *
 * 002AH: 空调代码号 (高位在前, 2 bytes)
 * 002BH: 状态字节1 + 状态字节2
 *        状态字节1: Bit7(开关) Bit6(风向) Bit5(灯光) Bit4(保留)
 *                    Bit3-Bit0: 温度 0-E (16-30℃)
 *        状态字节2: Bit7-4(模式) Bit3-0(风速)
 * 002CH: 状态字节3 + 状态字节4
 *        状态字节3: 键值 (0-电源,1-风向,3-温度+,4-温度-,5-模式,6-风速,7-灯光)
 *        状态字节4: Bit1(0-组合键,1-单一键) Bit0(0-默认码库,1-指定码库)
 * ================================================================ */

#define GMIRV2401_REG_AC_COMBO_CODE     0x002A  /** 空调组合-代码号 */
#define GMIRV2401_REG_AC_COMBO_STATE12  0x002B  /** 空调组合-状态12 */
#define GMIRV2401_REG_AC_COMBO_STATE34  0x002C  /** 空调组合-状态34 */

/* ================================================================
 * 电视控制寄存器 (002DH-0031H)
 * ================================================================ */

/** 电视电源 (翻转键) */
#define GMIRV2401_REG_TV_POWER          0x002D
/** 电视音量: 1-音量+, 0-音量- */
#define GMIRV2401_REG_TV_VOLUME         0x002E
/** 电视频道: 1-频道+, 0-频道- */
#define GMIRV2401_REG_TV_CHANNEL        0x002F
/** 电视信号源 */
#define GMIRV2401_REG_TV_SOURCE         0x0030
/** 电视返回 */
#define GMIRV2401_REG_TV_RETURN         0x0031

/* ================================================================
 * 机顶盒控制寄存器 (0032H-0039H)
 * ================================================================ */

/** 机顶盒组合状态 (暂不支持) */
#define GMIRV2401_REG_STB_COMBO_CODE    0x0032
#define GMIRV2401_REG_STB_COMBO_STATE12 0x0033
#define GMIRV2401_REG_STB_COMBO_STATE34 0x0034
/** 机顶盒电源 (翻转键) */
#define GMIRV2401_REG_STB_POWER         0x0035
/** 机顶盒音量 */
#define GMIRV2401_REG_STB_VOLUME        0x0036
/** 机顶盒频道 */
#define GMIRV2401_REG_STB_CHANNEL       0x0037
/** 机顶盒信号源 */
#define GMIRV2401_REG_STB_SOURCE        0x0038
/** 机顶盒返回 */
#define GMIRV2401_REG_STB_RETURN        0x0039

/* ================================================================
 * 互感器 / ADC 寄存器
 * ================================================================ */

/** 外接互感器设备开关状态: 0-关机, 1-开机 */
#define GMIRV2401_REG_CT_STATUS         0x003A
/** ADC 采样值 (10mV 单位) */
#define GMIRV2401_REG_ADC_VALUE         0x003B

/* ================================================================
 * PLC 组态空调寄存器 (0040H-0046H)
 * ================================================================ */

/** 本地码库代号 (默认发码代号) */
#define GMIRV2401_REG_PLC_AC_LOCAL_CODE 0x0040
/** 指令控制代号 (FFFFH=使用本地代号) */
#define GMIRV2401_REG_PLC_AC_CTRL_CODE  0x0041
/** 空调电源状态 (PLC): 1-开, 0-关 */
#define GMIRV2401_REG_PLC_AC_POWER      0x0042
/** 空调温度状态 (PLC): 16-31℃ */
#define GMIRV2401_REG_PLC_AC_TEMP       0x0043
/** 空调模式状态 (PLC): 0-自动 1-制冷 2-除湿 3-送风 4-制热 */
#define GMIRV2401_REG_PLC_AC_MODE       0x0044
/** 空调风速状态 (PLC): 0-自动 1-一档 2-二档 3-三档 */
#define GMIRV2401_REG_PLC_AC_FAN        0x0045
/** 空调灯光状态 (PLC): 0-关灯 1-开灯 */
#define GMIRV2401_REG_PLC_AC_LIGHT      0x0046

/* ================================================================
 * 红外学习通道寄存器 (共 64 通道: 0-63)
 *
 * 通道 0:     5000H-5031H (50 寄存器 = 100 bytes)
 * 通道 1-62:  0532H-114DH (每个通道 50 寄存器)
 * 通道 63:    114EH-1180H (50 寄存器)
 * ================================================================ */

#define GMIRV2401_REG_IR_CHANNEL_0      0x5000  /** 通道0 起始 */
#define GMIRV2401_REG_IR_CHANNEL_BASE   0x0532  /** 通道1 起始 */
#define GMIRV2401_REG_IR_CHANNEL_SIZE   50      /** 每通道寄存器数 */
#define GMIRV2401_IR_CHANNEL_MAX        64      /** 最大通道数 (0-63) */

/** 获取通道 N 的起始寄存器地址 (N: 0-63) */
#define GMIRV2401_IR_CHANNEL_REG(n)     (((n) == 0) ? GMIRV2401_REG_IR_CHANNEL_0 : (GMIRV2401_REG_IR_CHANNEL_BASE + (uint16_t)((n) - 1) * GMIRV2401_REG_IR_CHANNEL_SIZE))

/* ================================================================
 * 蓝牙透传寄存器
 * ================================================================ */

/** 蓝牙透传数据寄存器 */
#define GMIRV2401_REG_BLE_PASSTHROUGH   0x3000

/* ================================================================
 * 红外学习状态返回值
 * ================================================================ */

#define GMIRV2401_IR_LEARN_DONE         0x8002  /** 学习完成 */
#define GMIRV2401_IR_LEARN_TIMEOUT      0x8003  /** 学习超时 */

/* ================================================================
 * 空调匹配状态返回值
 * ================================================================ */

#define GMIRV2401_MATCH_START           0x0001  /** 启动匹配 */
#define GMIRV2401_MATCH_TIMEOUT         0x8003  /** 匹配超时/错误 */

/* ================================================================
 * 设备类型
 * ================================================================ */

#define GMIRV2401_DEV_TYPE_AC           0x00    /** 空调 */
#define GMIRV2401_DEV_TYPE_TV           0x01    /** 电视 */
#define GMIRV2401_DEV_TYPE_STB          0x02    /** 机顶盒 */

/* ================================================================
 * 按键 ID 定义 (附件3)
 * ================================================================ */

#define GMIRV2401_KEY_MUTE              0x00
#define GMIRV2401_KEY_SLEEP             0x01
#define GMIRV2401_KEY_HOME              0x02
#define GMIRV2401_KEY_DISPLAY           0x03
#define GMIRV2401_KEY_VOL_UP            0x04
#define GMIRV2401_KEY_CH_UP             0x05
#define GMIRV2401_KEY_CH_NUM            0x06
#define GMIRV2401_KEY_BACK              0x07
#define GMIRV2401_KEY_PAGE_UP           0x08
#define GMIRV2401_KEY_NAV_HOME          0x09
#define GMIRV2401_KEY_PAGE_DOWN         0x0A
#define GMIRV2401_KEY_TV                0x0B
#define GMIRV2401_KEY_VOD               0x0C
#define GMIRV2401_KEY_POWER             0x0D
#define GMIRV2401_KEY_VOL_DOWN          0x0E
#define GMIRV2401_KEY_CH_DOWN           0x0F
#define GMIRV2401_KEY_UP                0x10
#define GMIRV2401_KEY_DOWN              0x11
#define GMIRV2401_KEY_LEFT              0x12
#define GMIRV2401_KEY_RIGHT             0x13
#define GMIRV2401_KEY_OK                0x14
#define GMIRV2401_KEY_MENU              0x15
#define GMIRV2401_KEY_SOURCE            0x16
#define GMIRV2401_KEY_EXIT              0x17
#define GMIRV2401_KEY_0                 0x18
#define GMIRV2401_KEY_1                 0x19
#define GMIRV2401_KEY_2                 0x1A
#define GMIRV2401_KEY_3                 0x1B
#define GMIRV2401_KEY_4                 0x1C
#define GMIRV2401_KEY_5                 0x1D
#define GMIRV2401_KEY_6                 0x1E
#define GMIRV2401_KEY_7                 0x1F
#define GMIRV2401_KEY_8                 0x20
#define GMIRV2401_KEY_9                 0x21

/* ================================================================
 * 空调模式常量
 * ================================================================ */

#define GMIRV2401_AC_MODE_AUTO          0
#define GMIRV2401_AC_MODE_COOL          1
#define GMIRV2401_AC_MODE_DRY           2
#define GMIRV2401_AC_MODE_FAN           3
#define GMIRV2401_AC_MODE_HEAT          4

/* ================================================================
 * 空调风速常量
 * ================================================================ */

#define GMIRV2401_AC_FAN_AUTO           0
#define GMIRV2401_AC_FAN_LOW            1
#define GMIRV2401_AC_FAN_MED            2
#define GMIRV2401_AC_FAN_HIGH           3

/* ================================================================
 * 波特率常量
 * ================================================================ */

#define GMIRV2401_BAUD_2400             0
#define GMIRV2401_BAUD_4800             1
#define GMIRV2401_BAUD_9600             2
#define GMIRV2401_BAUD_14400            3
#define GMIRV2401_BAUD_19200            4
#define GMIRV2401_BAUD_38400            5
#define GMIRV2401_BAUD_56000            6
#define GMIRV2401_BAUD_57600            7
#define GMIRV2401_BAUD_115200           8

#ifdef __cplusplus
}
#endif

#endif /* GMIRV2401_REG_H */

