#include "motor.h"

const static uint8_t motor_pins[4] = {MOTOR_PIN_AP, MOTOR_PIN_AM, MOTOR_PIN_BP, MOTOR_PIN_BM};
const static uint8_t motor_step_table[8][4]={
    {1,0,0,0},
    {1,1,0,0},
    {0,1,0,0},
    {0,1,1,0},
    {0,0,1,0},
    {0,0,1,1},
    {0,0,0,1},
    {1,0,0,1}
};
//初始化：配置为输出模式，并全部设置为低电平
void motor_init(void){
  for(int i=0;i<4;i++){
    pinMode(motor_pins[i], OUTPUT);
    digitalWrite(motor_pins[i], LOW);
  }
  motor_release();
}
//释放所有线圈
void motor_release(void){
  for(int i=0;i<4;i++){
    digitalWrite(motor_pins[i], LOW);
  }
}

