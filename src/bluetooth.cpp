#include "bluetooth.h"

BluetoothSerial SerialBT;


void BT_start(void){

    if(!SerialBT.begin(BT_NAME)){
        SerialBT.println("启动失败");
        return;
    }
    SerialBT.println("蓝牙已启动\r\n设备名称: ");
    SerialBT.println(BT_NAME);

}

void BT_stop(){
    SerialBT.end();

}

static uint8_t recv_data;
uint8_t BT_recv_data[512][48];

void BT_recv(){
    if(SerialBT.available()){
        recv_data=SerialBT.read();

    }
}