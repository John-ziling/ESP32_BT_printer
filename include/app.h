#pragma once
// =====================================================
//  app.h —— 应用层服务：状态灯、空闲计时、自动关机（待机）
// =====================================================
#include <Arduino.h>

void     app_init(void);
void     app_touch(void);                      // 有活动：按键 / 收到数据 / 打印完成
void     app_set_busy(bool on);                // 打印中：状态灯常亮
void     app_wake(void);                       // 从待机唤醒
bool     app_in_standby(void);

void     app_set_power_down_min(uint16_t m);   // 0 = 不自动关机
uint16_t app_power_down_min(void);             // 供上位机查询

// 非阻塞：状态灯 + 自动关机判断
void     app_loop(bool paper_missing, bool low_bat, bool overtemp);
