#include <main.h>

void setup() {
  Serial.begin(115200); 
  delay(200);
  BT_Init();
  printinit();
  printer_clear();
  motor_init();
  LEDInit();
  BTNInit();
  papertest_Config();
  bt_print_init();
}


uint32_t count = 0;
void loop() {
  if(count>500){
    count = 0;
    getBatteryPower();
    test_head_temp();
  }
  count++;
  if(digitalRead(BTN_PIN) == LOW){
    delay(20);
    if(digitalRead(BTN_PIN) == LOW){
      if (papertest() == 0) {
        print_page((const uint8_t *)bitmap_page,
                   sizeof(bitmap_page) / sizeof(bitmap_page[0]),
                   HEAT_DEF_TIME_US);
      }
      while (digitalRead(BTN_PIN) == LOW) delay(10);
      delay(20);
    }
  }
  bt_print_poll();
  papertest();
  delay(1);
}
