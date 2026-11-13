#include "paper.h"

unsigned long paper_warning_time = 2000;

void papertest_Config(void)
{
    pinMode(PHINT, INPUT);
}
// 检测纸张状态,高电平为缺纸
uint8_t papertest(void)
{
    unsigned long currentTime=millis();
    static unsigned long previousTime = 0;

    if(digitalRead(PHINT) == HIGH)
    { 
        if ((currentTime - previousTime) > paper_warning_time)
        {
            previousTime = currentTime;
            Serial.print("打印机缺纸！\r\n");   
        }
        return 1;
    }
    else 
    {
        return 0;
    }
}


