#include "printer.h"

static SPISettings HEAD_SPI(1000000, MSBFIRST, SPI_MODE0); 
static const int STB_BUFFS[] = {STB1_PIN, STB2_PIN, STB3_PIN, STB4_PIN, STB5_PIN, STB6_PIN};

void printinit(void){
  pinMode(LAY_PIN, OUTPUT);
  digitalWrite(LAY_PIN, LOW);
  pinMode(VH_EN_PIN, OUTPUT);
  digitalWrite(VH_EN_PIN, LOW);
  for (int i = 0; i < HEAD_STB_NUM; i++) {
    pinMode(STB_BUFFS[i], OUTPUT);
    digitalWrite(STB_BUFFS[i], LOW);
  }
  SPI.begin(CLK_PIN, 4, DI_PIN, -1);//设置SPI引脚，CLK_PIN为时钟引脚，DI_PIN为数据输入引脚
}

//控制加热电源的开关
void printhead_vh(bool on) { 
  digitalWrite(VH_EN_PIN, on ? HIGH : LOW); 
} 


void printhead_send(const uint8_t row[HEAD_BYTES]) {
  SPI.beginTransaction(HEAD_SPI);
  SPI.writeBytes(row, HEAD_BYTES);   // 比 transfer 循环快
  SPI.endTransaction();

  digitalWrite(LAY_PIN, HIGH);
  delayMicroseconds(1);
  digitalWrite(LAY_PIN, LOW);
  
 
}

//清除打印数据
void printer_clear(){
  uint8_t clear_row[HEAD_BYTES] = {0};
  printhead_send(clear_row);
}

//按位设置选通信号
void printhead_stb(uint8_t mask){
  for(int i = 0; i < HEAD_STB_NUM; i++){
    if(mask&(1<<i)){
      digitalWrite(STB_BUFFS[i], HIGH);
    }else{
      digitalWrite(STB_BUFFS[i], LOW);
    }
  }
}

//指定部分加热函数
void printhead_heat_mask(uint8_t mask,uint32_t time_us){
  printhead_stb(mask);
  delayMicroseconds(time_us);
  printhead_stb(0);
}

//全部加热函数
void printhead_heat_all(uint32_t time_us){
  printhead_stb(0x3F); 
  delayMicroseconds(time_us);
  printhead_stb(0);
}


void printer_print_row(const uint8_t row[HEAD_BYTES], uint32_t heat_time_us){
  motor_run(4, false, 2000);
  delay(2);
  printhead_send(row);
  delayMicroseconds(1000);
  for (uint8_t i = 0; i < HEAD_STB_NUM; i++) {
    printhead_stb(1 << i);
    delayMicroseconds(heat_time_us);
    printhead_stb(0);
    delayMicroseconds(100);
  }
}

void printer_begin(void){
  printhead_vh(true);
  delay(5);
}

void printer_end(void){
  printhead_stb(0);
  printer_clear();
  printhead_vh(false);
}


void print_page(const uint8_t *data, uint32_t lines, uint32_t heat_us) {
  printer_begin();

  for (uint32_t line = 0; line < lines; line++) {
    printer_print_row(&data[line * HEAD_BYTES], heat_us);
  }

  motor_run(20 * 4, false, 2000); 
  motor_release();
  printer_end();
  motor_step_mm_false(10,1000);
}

