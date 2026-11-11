#include <main.h>

BluetoothSerial SerialBT;

// put function declarations here:

void setup() {
  Serial.begin(115200); 
  delay(200);
  printinit();
  printer_clear();
  motor_init();
  LEDconfig();
  BTNconfig();
  SerialBT.begin("喵喵机"); // 蓝牙设备名
}
uint32_t count = 0;
void loop() {

  // put your main code here, to run repeatedly:
  if (SerialBT.available()) {
    String data = SerialBT.readString();
    Serial.println(data);
  }
  if(count>500){
    count = 0;
    SerialBT.println("喵喵机");
  }
  count++;
  if(digitalRead(BTN_PIN) == LOW){
    delay(20);
    if(digitalRead(BTN_PIN) == LOW){
      motor_step_mm(50,1000);
    }
  }
  delay(20);
}
