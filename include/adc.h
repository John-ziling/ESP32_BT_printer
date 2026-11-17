#ifndef __ADC_H_
#define __ADC_H_

#define TEMP_ADC 36

#include "main.h"
#include "math.h"

void ADC_Config(void);

uint8_t head_temp_level(void);      // 0 正常 / 1 偏高(≥50℃) / 2 过高(≥80℃)，静默
uint8_t test_head_temp(void);       // 同上，但会把结果打印到串口（调试用）

// 原始温度（℃）：断线返回 -99，短路返回 -98
float get_head_temperature(void);

#endif
