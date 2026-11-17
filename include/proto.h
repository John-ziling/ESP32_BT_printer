#pragma once
// =====================================================
//  proto.h —— 喵喵机 App 指令表（2.0.3 版）
//  数值与"蓝牙打印机通信协议相关宏定义.txt"一致，向下兼容 1.0.2 版（26 条）
// =====================================================
#include <stdint.h>

enum : uint8_t {
    PRINT_DATA           = 0,   // 打印数据（位图，可分多包）
    PRINT_DATA_COMPRESS  = 1,   // 打印数据压缩（本项目不实现）
    FIRMWARE_DATA        = 2,   // 固件数据
    USB_UPDATE_FIRMWARE  = 3,   // USB 更新固件
    GET_VERSION          = 4,   // 获取版本
    SENT_VERSION         = 5,   // 发送版本
    GET_MODEL            = 6,   // 获取型号
    SENT_MODEL           = 7,   // 发送型号
    GET_BT_MAC           = 8,   // 获取蓝牙 MAC
    SENT_BT_MAC          = 9,   // 发送蓝牙 MAC
    GET_SN               = 10,  // 获取 SN
    SENT_SN              = 11,  // 发送 SN
    GET_STATUS           = 12,  // 获取状态
    SENT_STATUS          = 13,  // 发送状态
    GET_VOLTAGE          = 14,  // 获取电压
    SENT_VOLTAGE         = 15,  // 发送电压
    GET_BAT_STATUS       = 16,  // 获取电量状态
    SENT_BAT_STATUS      = 17,  // 发送电量状态
    GET_TEMP             = 18,  // 获取打印头温度
    SENT_TEMP            = 19,  // 发送打印头温度
    SET_FACTORY_STATUS   = 20,  // 设置出厂状态
    GET_FACTORY_STATUS   = 21,  // 获取出厂状态
    SENT_FACTORY_STATUS  = 22,  // 发送出厂状态
    SENT_BT_STATUS       = 23,  // 发送蓝牙状态
    SET_CRC_KEY          = 24,  // 设置 CRC 密钥
    SET_HEAT_DENSITY     = 25,  // 设定热密度
    FEED_LINE            = 26,  // 进料线 / 开始打印
    PRINT_TEST_PAGE      = 27,  // 打印测试页
    GET_HEAT_DENSITY     = 28,  // 获取热密度
    SENT_HEAT_DENSITY    = 29,  // 发送热密度
    SET_POWER_DOWN_TIME  = 30,  // 设定关机时间
    GET_POWER_DOWN_TIME  = 31,  // 获取断电时间
    SENT_POWER_DOWN_TIME = 32,  // 发送断电时间
    FEED_TO_HEAD_LINE    = 33,  // 送至打印头行
    PRINT_DEFAULT_PARA   = 34,  // 打印默认参数
    GET_BOARD_VERSION    = 35,  // 获取板子版本
    SENT_BOARD_VERSION   = 36,  // 发送板子版本
    GET_HW_INFO          = 37,  // 获取硬件信息
    SENT_HW_INFO         = 38,  // 发送硬件信息
    SET_MAX_GAP_LENGTH   = 39,  // 设定最大间隙长度
    GET_MAX_GAP_LENGTH   = 40,  // 获取最大间隙长度
    SENT_MAX_GAP_LENGTH  = 41,  // 发送最大间隙长度
    GET_PAPER_TYPE       = 42,  // 获取纸张类型
    SENT_PAPER_TYPE      = 43,  // 发送纸张类型
    SET_PAPER_TYPE       = 44,  // 设置纸张类型
    GET_COUNTRY_NAME     = 45,  // 获取国家名称
    SENT_COUNTRY_NAME    = 46,  // 发送国家名称
    DISCONNECT_BT        = 47,  // 断开蓝牙
    GET_DEV_NAME         = 48,  // 获取设备名称
    SENT_DEV_NAME        = 49,  // 发送设备名称

    // 厂商扩展指令
    CMD_39 = 57,
    CMD_40 = 64,
    CMD_41 = 65,
    CMD_42 = 66,
    CMD_43 = 67,
    CMD_7F = 127,
    CMD_80 = 128,
    CMD_81 = 129,
    CMD_82 = 130,
};

// 上位机（App）期望设备固定返回的内容
#define PROTO_SN            "P1001705253855"
#define PROTO_MODEL_NAME    "P1"
#define PROTO_COUNTRY       "CN"
#define PROTO_CMD_42_DATA   "BK3432"
#define PROTO_CRC_KEY       0x35769521u   // 厂商密钥，协议里的 crcKey（不参与 CRC 算法本身）
