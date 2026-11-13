#include "adc.h"

const float K = 273.15;
const float Temp_K = 298.15;
const float R25 = 30000;
const float B_VALUE = 3950.0; 
const float R_PULLUP = 10000.0;  // NTC 上拉电阻 10kΩ (原理图 R22)
const float VCC_MV = 3300.0;     // 系统供电 3.3V

extern BluetoothSerial SerialBT;

float battery_voltage = 0.0;
uint8_t battery_level = 0;
float head_temperature = 0.0;

void ADC_Config(void)
{
    pinMode(TEMP_ADC, INPUT);
}

static float get_adc_avg_mv(uint8_t pin){
    uint32_t sum=0;
    for(uint8_t i = 0; i < 10 ; i++){
        sum += analogReadMilliVolts(pin);
    }
    return sum/10;
}

float get_head_temperature(void)
{
    float pin_mv = get_adc_avg_mv(TEMP_ADC);
    
    // 断线或短路保护
    if (pin_mv <= 10.0) return -99.0; 
    if (pin_mv >= (VCC_MV - 10.0)) return -98.0;

    // 1. 算 NTC 当前阻值
    float r_ntc = R_PULLUP * pin_mv / (VCC_MV - pin_mv);
    
    // 2. B值公式算温度
    float log_val = log(r_ntc / R25);
    float inv_T = (1.0 / Temp_K) + (1.0 / B_VALUE) * log_val;
    
    if (inv_T == 0.0) return -97.0;
    
    float temp_k = 1.0 / inv_T;
    float temp_c = temp_k - K;
    
    return temp_c;
}

void test_head_temp(void){
    uint8_t temp = get_head_temperature();
    Serial.printf("当前温度：%d",temp);
    if(temp>=50 && temp<=80){
        Serial.printf("当前温度较高");
    }else if(temp>=80){
        Serial.printf("当前温度过高");
    }else{
        Serial.printf("当前温度正常");
    }
}