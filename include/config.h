#pragma once
#include <main.h>

//LED
#define LED_PIN 18
#define LED_ON() digitalWrite(LED_PIN, HIGH)
#define LED_OFF() digitalWrite(LED_PIN, LOW)
void LEDconfig(void);
//print head

#define HEAD_BYTES   48      // 一行 384 点 / 8  48字节
#define HEAD_DOTS    384 
#define HEAD_STB_NUM 6  //STB加热段个数
#define CLK_PIN 15
#define LAY_PIN 12
#define STB1_PIN 14
#define STB2_PIN 27
#define STB3_PIN 26
#define STB4_PIN 25
#define STB5_PIN 33
#define STB6_PIN 32
#define DI_PIN 13
#define VH_EN_PIN 17

//BTN
#define BTN_PIN 5
void BTNconfig(void);

