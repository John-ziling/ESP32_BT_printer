#include "motor.h"

static uint8_t motor_pins[4] = {MOTOR_PIN_AP, MOTOR_PIN_AM, MOTOR_PIN_BP, MOTOR_PIN_BM};

static const uint8_t motor_step_table[8][4] = {
    {1,0,0,0}, // A+
    {1,0,1,0}, // A+ B+
    {0,0,1,0}, // B+
    {0,1,1,0}, // A- B+
    {0,1,0,0}, // A-
    {0,1,0,1}, // A- B-
    {0,0,0,1}, // B-
    {1,0,0,1}  // A+ B-
};

static uint8_t motor_phase = 0; //当前相位

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
//应用相位设置电机线圈状态
static void apply_phase(uint8_t p) { 
  for (int i = 0; i < 4; i++) 
    digitalWrite(motor_pins[i], motor_step_table[p][i] ? HIGH : LOW); 
} 

//单步运动函数，phase为true表示顺时针，false表示逆时针，delay_us为每步的延迟时间
void motor_step(bool phase,uint32_t delay_us){
  if(phase){
    motor_phase=(uint8_t)((motor_phase + 1) & 7);
  }
  else{
    motor_phase=(uint8_t)((motor_phase + 7) & 7);
  }
  apply_phase(motor_phase);
  delayMicroseconds(delay_us);
}
 
//多步运动函数，steps为步数，phase为true表示顺时针，false表示逆时针，delay_us为每步的延迟时间
void motor_steps(uint32_t steps,bool phase,uint32_t delay_us){  
  for(uint32_t i=0;i<steps;i++){
    motor_step(phase,delay_us);
  }
  motor_release(); 
}

//按毫米运动函数，mm为毫米数，delay_us为每步的延迟时间
void motor_step_mm_true(float mm,uint32_t delay_us){
    int32_t steps = (int32_t)(mm * MOTOR_STEP_MM + 0.5f);
    motor_steps(steps, true, delay_us);
}

void motor_step_mm_false(float mm,uint32_t delay_us){
    int32_t steps = (int32_t)(mm * MOTOR_STEP_MM + 0.5f);
    motor_steps(steps, false, delay_us);
}
