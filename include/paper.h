#ifndef __PARPER_H_
#define __PARPER_H_

#include <Arduino.h>
#include "config.h"

// 缺纸检测：反射式光电 + LM393，高电平表示缺纸
#define PHINT HEAD_PHINT

void    papertest_Config(void);
uint8_t papertest(void);          // 返回 1 = 缺纸，0 = 有纸；内部 2 秒一次告警

#endif
