#include <Arduino.h>
#include <BluetoothSerial.h>

#define LED_PIN 2
#define LED_ON() digitalWrite(LED_PIN, HIGH)
#define LED_OFF() digitalWrite(LED_PIN, LOW)

BluetoothSerial SerialBT;

// put function declarations here:
void LEDconfig() {
  pinMode(LED_PIN, OUTPUT);
}

void setup() {
  Serial.begin(115200); 
  delay(200);
  Serial.println(); 
  Serial.println("=== STAGE 00 · blink + serial ==="); 
  Serial.printf("chip   : %s rev%d, %d cores @ %u MHz\n", 
                ESP.getChipModel(), ESP.getChipRevision(), 
                ESP.getChipCores(), ESP.getCpuFreqMHz()); 
  Serial.printf("flash  : %u KB\n", ESP.getFlashChipSize() / 1024); 
  Serial.printf("heap   : %u bytes free\n", ESP.getFreeHeap()); 

  LEDconfig();
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
