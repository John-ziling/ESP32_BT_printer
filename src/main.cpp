#include <main.h>

BluetoothSerial SerialBT;

// put function declarations here:
uint8_t test_row[HEAD_BYTES] = {0};

void setup() {
  memset(test_row, 0xFF, sizeof(test_row));
  Serial.begin(115200); 
  delay(200);
  printinit();
  printer_clear();
  motor_init();
  LEDconfig();
  BTNconfig();
  printhead_vh(true);
  SerialBT.begin("喵喵机"); // 蓝牙设备名
}
uint32_t count = 0;

void loop() {
  while (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  if(count>500){
    count = 0;
    SerialBT.println("喵喵机");
  }
  count++;
  if(digitalRead(BTN_PIN) == LOW){
    delay(20);
    if(digitalRead(BTN_PIN) == LOW){
       printer_print_row(test_row, HEAT_DEF_TIME_US);
      //motor_step_mm_true(100,1000);
      motor_step_mm_false(10,1000);
      while (digitalRead(BTN_PIN) == LOW) delay(10);
      delay(20);
    }
  }
  delay(20);
}
