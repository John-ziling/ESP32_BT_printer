#include <main.h>
#include "bt_print.h"
#include "printjob.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  ADC_Config();          // 打印头 NTC 引脚
  papertest_Config();    // 缺纸检测
  printinit();           // 打印头 SPI/锁存/STB
  printer_clear();       // 上电先清移位寄存器，防止随机数据加热
  motor_init();          // 步进电机
  LEDInit();
  BTNInit();
  app_init();            // 状态灯与空闲计时

  BT_Init();
  bt_print_init();
}

static uint32_t last_sensor_ms = 0;
static bool     paper_missing  = false;
static bool     head_overtemp  = false;

static void handle_button(void){
  delay(20);                                     // 去抖
  if (digitalRead(BTN_PIN) != LOW) return;

  if (papertest() != 0) {
    Serial.println("缺纸，按键只走纸不打印");
  }
  motor_step_mm_false(10, 1000);                 // 手动送纸 10mm
  motor_release();
  app_touch();

  while (digitalRead(BTN_PIN) == LOW) delay(10); // 等松手
  delay(20);
}

void loop() {
  uint32_t now = millis();
  static bool was_standby = false;
  bool standby = app_in_standby();

  // 刚进入待机：关蓝牙，进一步省电
  if (standby && !was_standby) {
    BT_Stop();
    Serial.println("已进入待机，蓝牙关闭");
  }
  was_standby = standby;

  // 待机：只等按键唤醒
  if (standby) {
    if (digitalRead(BTN_PIN) == LOW) {
      delay(20);
      if (digitalRead(BTN_PIN) == LOW) {
        Serial.println("按键唤醒");
        app_wake();
        BT_Init();                                  // 重新开启蓝牙
        bt_print_init();                            // 重置协议状态
        while (digitalRead(BTN_PIN) == LOW) delay(10);
      }
    }
    delay(20);
    return;
  }

  if (digitalRead(BTN_PIN) == LOW) handle_button();

  bt_print_poll();

  // 每 500ms 采样一次电池与打印头温度（ADC 采样较重，别每圈都做）
  if (now - last_sensor_ms >= 500) {
    last_sensor_ms = now;
    getBatteryPower(false);
    head_overtemp = (head_temp_level() == 2);
  }

  paper_missing = (papertest() != 0);             // 内部自带 2 秒告警节流
  app_loop(paper_missing, battery_low(), head_overtemp);

  delay(1);
}
