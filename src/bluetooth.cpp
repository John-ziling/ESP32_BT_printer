#include "bluetooth.h"

BluetoothSerial SerialBT;


void BT_Init(void){
    if(!SerialBT.begin(BT_NAME)){
        Serial.println("蓝牙启动失败");
        return;
    }
    Serial.println("蓝牙已启动，设备名称：");
    Serial.println(BT_NAME);
}

void BT_Stop(){
    SerialBT.end();
}