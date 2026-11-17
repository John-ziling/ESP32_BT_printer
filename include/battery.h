#pragma once

#include <main.h>

#define POWER_ADC_PIN 34

// 电压阈值（18650 单节）
#define BAT_FULL_V   4.20f    // 满电
#define BAT_STOP_V   3.55f    // 低于此值不允许开始打印
#define BAT_ABORT_V  3.40f    // 打印中低于此值立即中止（防掉压复位）
#define BAT_LOW_V    3.60f    // 低电告警

extern float   BATV_VOLT;         // 最近一次采样得到的电池电压（V）
extern uint8_t PRINTER_BATTERY;   // 最近一次采样得到的电量（0-100）

void  getBatteryPower(bool verbose = false);   // 采样并更新上面两个变量
bool  battery_low(void);                       // 是否处于低电告警区
