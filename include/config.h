#pragma once

//LED
#define LED_PIN 18
#define LED_ON()  digitalWrite(LED_PIN, LOW)    // 低电平点亮
#define LED_OFF() digitalWrite(LED_PIN, HIGH)
void LEDInit(void);

//print head
#define HEAD_PHINT   35
#define HEAD_BYTES   48      // 一行 384 点 / 8
#define HEAD_DOTS    384
#define HEAD_STB_NUM 6       // STB 加热段个数
#define CLK_PIN 15
#define STB1_PIN 14
#define STB2_PIN 27
#define STB3_PIN 26
#define STB4_PIN 25
#define STB5_PIN 33
#define STB6_PIN 32
#define DI_PIN 13

// BOARD_1 引脚定义（当前硬件）
#define LAY_PIN   12   // 锁存，空闲低 / 脉冲高
#define VH_EN_PIN 17   // 打印头升压电源使能

#define BTN_PIN 5

void BTNInit(void);

#include <Arduino.h>
