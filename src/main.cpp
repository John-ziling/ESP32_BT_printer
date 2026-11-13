#include <main.h>

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200); 
  delay(200);
  printinit();
  printer_clear();
  motor_init();
  LEDconfig();
  BTNconfig();
  pappertest_Config();
  printhead_vh(true);
  SerialBT.begin("喵喵机"); // 蓝牙设备名
}
uint32_t count = 0;

void loop() {
  while (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
  papertest();
  if(count>500){
    count = 0;
    getBatteryPower();
    test_head_temp();
    SerialBT.println("喵喵机");
  }
  count++;
  if(digitalRead(BTN_PIN) == LOW){
    delay(20);
    if(digitalRead(BTN_PIN) == LOW){
      motor_step_mm_false(10,1000);
      while (digitalRead(BTN_PIN) == LOW) delay(10);
      delay(20);
    }
  }
  delay(10);
}
