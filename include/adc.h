#ifndef __ADC_H_
#define __ADC_H_

#define TEMP_ADC  36

#include "main.h"
#include "math.h"

void ADC_Config(void);
uint8_t test_head_temp(void);
float get_head_temperature(void);   // 原始温度（℃），断线 -99 / 短路 -98

#endif
