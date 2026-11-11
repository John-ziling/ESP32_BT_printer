#include <Arduino.h>
#include <BluetoothSerial.h>
#include <SPI.h>
#define LED_PIN 2
#define LED_ON() digitalWrite(LED_PIN, HIGH)
#define LED_OFF() digitalWrite(LED_PIN, LOW)

static SPISettings HEAD_SPI(1000000, MSBFIRST, SPI_MODE0); 

BluetoothSerial SerialBT;

// put function declarations here:
void LEDconfig() {
  pinMode(LED_PIN, OUTPUT);
}
void KEYconfig() {
  pinMode(5, INPUT_PULLUP);
}

void setup() {
  Serial.begin(115200); 
  delay(200);
  SPI.beginTransaction(HEAD_SPI);
  
  SPI.begin();
  LEDconfig();
  KEYconfig();
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
}
