#pragma once


//LED
#define LED_PIN 18
#define LED_ON() digitalWrite(LED_PIN, LOW)
#define LED_OFF() digitalWrite(LED_PIN, HIGH)
void LEDconfig(void);
//print head
#define HEAD_PHINT   35
#define HEAD_BYTES   48      // 一行 384 点 / 8  48字节
#define HEAD_DOTS    384 
#define HEAD_STB_NUM 6  //STB加热段个数
#define CLK_PIN 15
#define STB1_PIN 14
#define STB2_PIN 27
#define STB3_PIN 26
#define STB4_PIN 25
#define STB5_PIN 33
#define STB6_PIN 32
#define DI_PIN 13
#define PIN_TEMP    36 

#define USE_BOARD_1 


#ifdef USE_BOARD_1 
#define LAY_PIN 12
#define VH_EN_PIN 17
#endif

#ifdef USE_BOARD_2
#define LAY_PIN 17
#define VH_EN_PIN 28
#endif

#define BTN_PIN 5

#define PIN_BATTEST 34

void BTNconfig(void);
#include <Arduino.h>