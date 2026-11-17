#include "printjob.h"

#include "printer.h"
#include "motor.h"
#include "paper.h"
#include "adc.h"
#include "battery.h"

// =====================================================
//  printjob.cpp —— 整页缓冲 + 打印执行
//
//  保护策略（按优先级）：
//    ① 缺纸：绝对不加热，打印前中止
//    ② 低电：<3.55V 拒绝开始；打印中 <3.40V 立即中止（防掉压复位）
//    ③ 过温：≥80℃ 中止；50~80℃ 线性降低加热时间
// =====================================================

#define TEST_LINES 160            // 自检图案 160 行 = 20mm

static uint8_t *s_page = nullptr;     // 页缓冲（按需扩容）
static uint32_t s_cap  = 0;
static uint32_t s_len  = 0;
static uint8_t  s_density = 64;       // 热密度（App 默认 64）
static bool     s_valid = true;       // 本页是否有效（溢出/内存不足则作废）

// 热密度 → 基础加热时间：0→1500µs，100→3500µs
static uint32_t base_heat_us(void){
    uint32_t us = 1500u + (uint32_t)s_density * 20u;
    if (us > HEAT_MAX_TIME_US) us = HEAT_MAX_TIME_US;
    return us;
}

// 按打印头温度补偿：≥50℃ 起线性缩短，80℃ 时约为原值的 70%
// （热敏纸在高温下更容易发色，不补偿会越打越黑甚至糊成一片）
static uint32_t heat_us_compensated(void){
    float t = get_head_temperature();
    float us = (float)base_heat_us();

    if (t > 50.0f) {
        float k = 1.0f - (t - 50.0f) / 30.0f * 0.3f;
        if (k < 0.70f) k = 0.70f;
        us *= k;
    }
    if (us < 600.0f)  us = 600.0f;
    if (us > (float)HEAT_MAX_TIME_US) us = (float)HEAT_MAX_TIME_US;
    return (uint32_t)us;
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
    s_len   = 0;
    s_valid = true;
    if (!ensure_cap(HEAD_BYTES * 32)) {     // 先要 32 行，不够再涨
        s_valid = false;
        return false;
    }
    return true;
}

void job_reset(void){
    s_len   = 0;
    s_valid = true;
}

bool job_append(const uint8_t *d, uint16_t n){
    if (n == 0) return true;
    if (!s_valid) return false;

    uint32_t need = s_len + n;
    if (need > PAGE_MAX_BYTES) {
        Serial.printf("页缓冲溢出：已有 %u，又来 %u，本页作废\n",
                      (unsigned)s_len, (unsigned)n);
        s_valid = false;
        return false;
    }
    if (!ensure_cap(need)) { s_valid = false; return false; }

    memcpy(s_page + s_len, d, n);
    s_len = need;
    return true;
}

uint32_t job_bytes(void){ return s_len; }
uint32_t job_lines(void){ return s_len / HEAD_BYTES; }
bool     job_is_valid(void){ return s_valid; }

void job_feed_mm(float mm){
    motor_step_mm_false(mm, MOTOR_STEP_US);
    motor_release();
}

bool job_print(void){
    uint32_t lines = job_lines();

    if (!s_valid) { Serial.println("本页数据不完整（曾溢出），拒绝打印"); return false; }
    if (lines == 0) return true;

    // ① 缺纸绝不加热
    if (papertest() != 0) { Serial.println("缺纸，取消打印"); return false; }

    // ② 低电不允许开始（打印瞬间电流可达 2.7A，电压不够会直接复位）
    getBatteryPower(false);
    if (BATV_VOLT < BAT_STOP_V) {
        Serial.printf("电量不足（%.2fV），取消打印\n", BATV_VOLT);
        return false;
    }

    uint32_t heat = heat_us_compensated();
    uint32_t done = 0;

    Serial.printf("开始打印：%u 行，加热 %u µs\n", (unsigned)lines, (unsigned)heat);
    printer_begin();

    for (uint32_t i = 0; i < lines; i++) {
        if (papertest() != 0) { Serial.println("打印中缺纸，中止"); break; }

        // 过温：≥80℃ 立即中止。每 8 行查一次即可（每次要采 10 次 ADC）
        if ((i & 7) == 0 && head_temp_level() == 2) {
            Serial.println("打印头过热，中止");
            break;
        }

        // 每 32 行复核一次电压，防止打印途中掉压复位
        if ((i & 31) == 0) {
            getBatteryPower(false);
            if (BATV_VOLT < BAT_ABORT_V) {
                Serial.printf("打印中电量过低（%.2fV），中止\n", BATV_VOLT);
                break;
            }
        }

        printer_print_row(&s_page[i * HEAD_BYTES], heat);
        done++;
    }

    motor_run(20 * 4, false, 2000);   // 尾部走纸
    motor_release();
    printer_end();
    job_feed_mm(5);                   // 送到撕纸口

    Serial.printf("打印完成 %u/%u 行\n", (unsigned)done, (unsigned)lines);

    // 无论成功还是中止都清空本页：避免一次中止后旧数据被再次打印
    job_reset();
    return done == lines;
}

void job_set_density(uint8_t d){
    s_density = (d > 100) ? 100 : d;
    Serial.printf("热密度 = %u（加热 %u µs）\n", (unsigned)s_density,
                  (unsigned)base_heat_us());
}

uint8_t job_get_density(void){ return s_density; }

// -----------------------------------------------------
//  内置自检图案：外框 + 每 6 行点亮一个 STB 段（阶梯）
//  6 段依次点亮，哪一段不工作，纸面上对应的那一列就会缺失
// -----------------------------------------------------
static void set_dot(uint8_t *row, uint32_t x){
    if (x < HEAD_DOTS) row[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
}

void job_render_test_page(void){
    job_begin();

    uint8_t row[HEAD_BYTES];
    for (uint32_t y = 0; y < TEST_LINES; y++) {
        memset(row, 0, sizeof(row));

        bool frame = (y < 2) || (y >= TEST_LINES - 2);
        bool sep   = ((y % 20) < 2);

        if (frame || sep) {
            memset(row, 0xFF, sizeof(row));            // 外框与横向分隔线
        } else {
            uint32_t band = (y / 6) % HEAD_STB_NUM;    // 阶梯：每 6 行换一段
            for (uint32_t x = band * 64; x < band * 64 + 64; x++) set_dot(row, x);
            set_dot(row, 0);
            set_dot(row, HEAD_DOTS - 1);
        }
        job_append(row, HEAD_BYTES);
    }
}
