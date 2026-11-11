#pragma once
#include <main.h>

#define MOTOR_PIN_AP 22
#define MOTOR_PIN_AM 23  
#define MOTOR_PIN_BP 19
#define MOTOR_PIN_BM 21

#define MOTOR_STEP_MM 32 //每毫米的步数，32步/毫米
#define MOTOR_STEP_US 1000 //每步的延迟时间，单位微秒


void motor_init(void);
void motor_release(void);
void motor_step(bool phase,uint32_t delay_us);
void motor_steps(uint32_t steps,bool phase,uint32_t delay_us);
void motor_step_mm(float mm,uint32_t delay_us);
