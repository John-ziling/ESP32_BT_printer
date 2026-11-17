#include "app.h"
#include "config.h"

// =====================================================
//  app.cpp —— 状态灯 + 自动关机
//
//  状态灯（GPIO18，低电平点亮）：
//    打印中        常亮
//    缺纸 / 过温   快闪（200ms）
//    低电          慢闪（1s）
//    其他          熄灭
//
//  自动关机：空闲超过设定分钟数后进入待机（关蓝牙、关打印头电源、灭灯），
//  按键唤醒后重新初始化蓝牙。不做 deep sleep —— 按键 GPIO5 不是 RTC 脚，
//  进了深睡就只能手动复位，体验更差。
// =====================================================

#define LED_BLINK_FAST_MS 200
#define LED_BLINK_SLOW_MS 1000

static uint32_t s_last_active    = 0;
static uint16_t s_power_down_min = 60;
static bool     s_busy           = false;
static bool     s_standby        = false;
static bool     s_led_on         = false;
static uint32_t s_led_tick       = 0;

void app_init(void){
    s_last_active    = millis();
    s_power_down_min = 60;
    s_busy           = false;
    s_standby        = false;
    s_led_on         = false;
    s_led_tick       = s_last_active;
    LED_OFF();
}

void app_touch(void){ s_last_active = millis(); }

void app_set_busy(bool on){
    s_busy = on;
    if (on) LED_ON(); else LED_OFF();
}

void app_set_power_down_min(uint16_t m){ s_power_down_min = m; }
uint16_t app_power_down_min(void){ return s_power_down_min; }

void app_wake(void){
    s_standby     = false;
    s_last_active = millis();
}

bool app_in_standby(void){ return s_standby; }

static void led_write(bool on){
    s_led_on = on;
    if (on) LED_ON(); else LED_OFF();
}

void app_loop(bool paper_missing, bool low_bat, bool overtemp){
    uint32_t now = millis();

    if (s_standby) return;

    // 自动关机：空闲超时且没在打印
    if (!s_busy && s_power_down_min > 0 &&
        (now - s_last_active) > (uint32_t)s_power_down_min * 60000u) {
        s_standby = true;
        led_write(false);
        Serial.printf("空闲 %u 分钟，进入待机\n", (unsigned)s_power_down_min);
        return;
    }

    if (s_busy) {
        if (!s_led_on) led_write(true);
        return;
    }

    uint32_t period = 0;
    if (paper_missing || overtemp) period = LED_BLINK_FAST_MS;
    else if (low_bat)              period = LED_BLINK_SLOW_MS;

    if (period == 0) {
        if (s_led_on) led_write(false);
        return;
    }

    if (now - s_led_tick >= period) {
        s_led_tick = now;
        led_write(!s_led_on);
    }
}
