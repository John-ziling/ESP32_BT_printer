#include "battery.h"

uint16_t adc=0;

static uint16_t get_adc_avg(uint8_t pin){
    uint32_t sum=0;
    for(uint8_t i = 0; i < 10 ; i++){
        sum += analogReadMilliVolts(pin);
    }
    return sum/10;
}

void getBatteryPower(void)
{  
    uint16_t pin_mv = 0;
    uint16_t now_vbat = 0;
    uint8_t Battery_level=0;
    pin_mv=get_adc_avg(POWER_ADC_PIN);
    now_vbat=pin_mv*2/1000;

    Serial.printf("引脚读取的mv值: %d,当前电压：%d",pin_mv,now_vbat);
    for(int i= sizeof(batVoltagelist)/sizeof(batVoltagelist[0])-1;i >= 0 ;i--){
        if(now_vbat >= batVoltagelist[i]){
            Battery_level=i;
            break;
        }
    }
    Serial.printf("查表获取的电量：%d%%",Battery_level);
    
}