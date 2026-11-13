#include "paper.h"

unsigned long paper_warning_time = 2000;

void pappertest_Config(void)
{
    pinMode(PHINT, INPUT);
}

uint8_t papertest(void)
{
    unsigned long currentTime=millis();
    static unsigned long previousTime = 0;

    if(digitalRead(PHINT) == HIGH)
    { 
        if ((currentTime - previousTime) > paper_warning_time)
        {
            previousTime = currentTime;
            Serial.print(" paper out \r\n");   
        }
        return 1;
    }
    else 
    {
        return 0;
    }
}


