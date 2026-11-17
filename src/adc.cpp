#include "adc.h"

// =====================================================
//  adc.cpp —— 打印头 NTC 测温
//
//  电路：3V3 —[R22 10k]— THER(GPIO36) —[NTC]— GND
//  参数：R25 = 30kΩ ±5%，B = 3950K ±2%（JX-2R-01 规格书 §3.5）
//  公式：Rntc = R上拉 × Vadc / (Vcc − Vadc)
//        1/T  = 1/T25 + (1/B)·ln(Rntc / R25)
//  参考点：25℃ 时引脚电压约 2475 mV（30k/40k × 3.3V），可用来校表
// =====================================================

static const float K        = 273.15f;   // 开尔文偏移
static const float TEMP_25K = 298.15f;   // 25℃ 对应的绝对温度
static const float R25      = 30000.0f;  // NTC 在 25℃ 的阻值
static const float B_VALUE  = 3950.0f;   // B 常数
static const float R_PULLUP = 10000.0f;  // 板上上拉电阻
static const float VCC_MV   = 3300.0f;   // 系统供电

void ADC_Config(void){
    pinMode(TEMP_ADC, INPUT);
}

static float get_adc_avg_mv(uint8_t pin){
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 10; i++) {
        sum += analogReadMilliVolts(pin);
    }
    return (float)(sum / 10);
}

float get_head_temperature(void){
    float pin_mv = get_adc_avg_mv(TEMP_ADC);

    // 断线 / 短路保护：算下去会得到离谱阻值
    if (pin_mv <= 10.0f)             return -99.0f;
    if (pin_mv >= (VCC_MV - 10.0f))  return -98.0f;

    // 1. 由分压反推 NTC 阻值
    float r_ntc = R_PULLUP * pin_mv / (VCC_MV - pin_mv);

    // 2. B 值公式求绝对温度，再换成摄氏度
    float inv_t = (1.0f / TEMP_25K) + (1.0f / B_VALUE) * log(r_ntc / R25);
    if (inv_t == 0.0f) return -97.0f;

    return (1.0f / inv_t) - K;
}

// 静默版：打印流程每行都会调用，不能打串口
uint8_t head_temp_level(void){
    float t = get_head_temperature();
    if (t >= 80.0f) return 2;
    if (t >= 50.0f) return 1;
    return 0;
}

uint8_t test_head_temp(void){
    float t  = get_head_temperature();
    uint8_t lv = head_temp_level();

    if (lv == 2)      Serial.printf("打印头温度 %.1f℃：过高\n", t);
    else if (lv == 1) Serial.printf("打印头温度 %.1f℃：偏高\n", t);

    return lv;
}
