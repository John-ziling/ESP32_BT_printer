#include <main.h>

BluetoothSerial SerialBT;

// put function declarations here:

void setup() {
  Serial.begin(115200); 
  delay(200);
  SPI.begin();
  printinit();
  printer_clear();
  LEDconfig();
  BTNconfig();
  SerialBT.begin("喵喵机"); // 蓝牙设备名
}

void loop() {
  // put your main code here, to run repeatedly:
  if (SerialBT.available()) {
    String data = SerialBT.readString();
    Serial.println(data);
  }
  SerialBT.println("喵喵机运行中!");
  
  delay(1000);
  delayMicroseconds(1000);
}
