#include "printjob.h"

#include "printer.h"
#include "motor.h"
#include "paper.h"
#include "adc.h"

// =====================================================
//  printjob.cpp —— 整页缓冲 + 打印执行
// =====================================================

static uint8_t *s_page = nullptr;     // 页缓冲（按需扩容）
static uint32_t s_cap  = 0;
static uint32_t s_len  = 0;
static uint8_t  s_density = 64;       // 热密度（App 默认 64）
static bool     s_busy = false;

// 热密度 → 每段加热时间：0→1500µs，100→3500µs
static uint32_t heat_us_from_density(void){
    uint32_t us = 1500u + (uint32_t)s_density * 20u;
    if (us > HEAT_MAX_TIME_US) us = HEAT_MAX_TIME_US;
    return us;
}

// 按 8KB 步长翻倍扩容，小页不吃满 64KB
static bool ensure_cap(uint32_t need){
    if (need <= s_cap) return true;

    uint32_t cap = s_cap ? s_cap : 8192u;
    while (cap < need) cap <<= 1;
    if (cap > PAGE_MAX_BYTES) cap = PAGE_MAX_BYTES;
    if (cap < need) return false;

    uint8_t *p = (uint8_t *)realloc(s_page, cap);
    if (!p) {
        Serial.printf("页缓冲扩容失败（需要 %u 字节）\n", (unsigned)cap);
        return false;
    }
    s_page = p;
    s_cap  = cap;
    return true;
}

bool job_begin(void){
    s_len = 0;
    return ensure_cap(HEAD_BYTES * 32);      // 先要 32 行，不够再涨
}

void job_reset(void){
    s_len = 0;
}

bool job_append(const uint8_t *d, uint16_t n){
    if (n == 0) return true;

    // 只收整行：行尾不足一行的部分先忽略（协议里分包都是 48 的整数倍）
    uint32_t need = s_len + n;
    if (need > PAGE_MAX_BYTES) {
        Serial.printf("页缓冲溢出：已有 %u，又来 %u\n", (unsigned)s_len, (unsigned)n);
        return false;
    }
    if (!ensure_cap(need)) return false;

    memcpy(s_page + s_len, d, n);
    s_len = need;
    return true;
}

uint32_t job_bytes(void){ return s_len; }
uint32_t job_lines(void){ return s_len / HEAD_BYTES; }

void job_feed_mm(float mm){
    motor_step_mm_false(mm, MOTOR_STEP_US);
    motor_release();
}

bool job_print(void){
    uint32_t lines = job_lines();
    if (lines == 0) return true;

    if (papertest() != 0) {
        Serial.println("缺纸，取消打印");
        return false;
    }

    s_busy = true;
    uint32_t heat = heat_us_from_density();
    uint32_t done = 0;

    printer_begin();

    for (uint32_t i = 0; i < lines; i++) {
        if (papertest() != 0) { Serial.println("打印中缺纸，中止"); break; }

        // 每 8 行查一次温度（test_head_temp 会打串口，别每行都查）
        if ((i & 7) == 0 && test_head_temp() == 2) {
            Serial.println("打印头过热，中止");
            break;
        }

        printer_print_row(&s_page[i * HEAD_BYTES], heat);
        done++;
    }

    motor_run(20 * 4, false, 2000);   // 尾部走纸
    motor_release();
    printer_end();
    job_feed_mm(5);                   // 送到撕纸口

    s_busy = false;
    Serial.printf("打印完成 %u/%u 行\n", (unsigned)done, (unsigned)lines);
    return done == lines;
}

void job_set_density(uint8_t d){
    s_density = (d > 100) ? 100 : d;
    Serial.printf("热密度 = %u\n", (unsigned)s_density);
}

uint8_t job_get_density(void){ return s_density; }
