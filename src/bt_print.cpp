#include "bt_print.h"
#include "frame.h"
#include "proto.h"
#include "printjob.h"

// =====================================================
//  bt_print.cpp —— 喵喵机上位机适配
//
//  收：0x02 | type | idx | len(2,LE) | payload | CRC32 | 0x03
//  回：查询类回 SENT_* 数据帧；其余回 11 字节 ACK
//  CRC 错的帧不回任何东西，让 App 超时重发
//
//  握手顺序（App 连上后依次发）：
//    24 SET_CRC_KEY → 10 GET_SN → 66 → 127 → 25 SET_HEAT_DENSITY
//    → 48 GET_DEV_NAME → 16 GET_BAT_STATUS → 31 GET_POWER_DOWN_TIME
//    → 4 GET_VERSION → 44 SET_PAPER_TYPE
//
//  打印流程：
//    25 ×2 → 26 FEED_LINE（开始）→ 0 PRINT_DATA ×N → 26 FEED_LINE（打印）
// =====================================================

#ifndef PROTO_REPLY_ACK_FOR_QUERY
#define PROTO_REPLY_ACK_FOR_QUERY 0   // 1 = 查询类也先回 ACK（App 不认数据帧时再打开）
#endif

#define POLL_BYTE_BUDGET 512

static FrameParser s_parser;
static Frame       s_frame;

static bool     s_armed = false;         // 已收到"开始"信号
static uint16_t s_power_down_min = 3600; // 自动关机时间（分钟），App 默认要 3600

// ---------- 发送 ----------
// 不要用 availableForWrite()：这版 BluetoothSerial 没实现，永远返回 0
static bool bt_send(const uint8_t *buf, size_t len){
    size_t   sent  = 0;
    uint16_t retry = 0;

    while (sent < len) {
        size_t chunk = len - sent;
        if (chunk > 512) chunk = 512;

        size_t n = SerialBT.write(buf + sent, chunk);
        sent += n;

        if (n < chunk) {
            if (!SerialBT.hasClient()) return false;
            if (++retry > 300) return false;
            delay(1);
        } else {
            retry = 0;
        }
    }
    return true;
}

static void reply_ack(uint8_t type){
    uint8_t buf[ACK_LEN];
    ack_build(type, buf);
    bt_send(buf, ACK_LEN);
}

static void reply_data(uint8_t type, const uint8_t *payload, uint16_t len){
    uint8_t buf[96];
    uint16_t total = frame_build(type, 0x00, payload, len, buf, sizeof(buf));
    if (total) bt_send(buf, total);
}

// ---------- 指令处理 ----------
static void handle_frame(void){
    const uint8_t *p = s_frame.payload;
    uint16_t       n = s_frame.len;
    uint8_t        t = s_frame.type;

    // 每行都回一次 ACK 的开关（调试用）
    if (PROTO_REPLY_ACK_FOR_QUERY) reply_ack(t);

    switch (t) {
    // ---------- 查询类：回数据帧 ----------
    case GET_VERSION: {
        static const uint8_t v[3] = {0x01, 0x00, 0x02};        // 2.0.1
        reply_data(SENT_VERSION, v, sizeof(v));
        break;
    }
    case GET_DEV_NAME:
        reply_data(SENT_DEV_NAME, (const uint8_t *)PROTO_MODEL_NAME, 2);
        break;
    case GET_SN:
        reply_data(SENT_SN, (const uint8_t *)PROTO_SN, (uint16_t)strlen(PROTO_SN));
        break;
    case GET_COUNTRY_NAME:
        reply_data(SENT_COUNTRY_NAME, (const uint8_t *)PROTO_COUNTRY, 2);
        break;
    case GET_BAT_STATUS: {
        getBatteryPower();
        reply_data(SENT_BAT_STATUS, &PRINTER_BATTERY, 1);
        break;
    }
    case GET_VOLTAGE: {
        getBatteryPower();
        uint16_t mv = (uint16_t)(BATV_VOLT * 1000.0f);
        uint8_t  b[2] = {(uint8_t)(mv & 0xFF), (uint8_t)(mv >> 8)};
        reply_data(SENT_VOLTAGE, b, 2);
        break;
    }
    case GET_TEMP: {
        float    th = get_head_temperature();
        int32_t  c  = (int32_t)(th + 0.5f);
        uint8_t  b  = (uint8_t)(c < 0 ? 0 : (c > 255 ? 255 : c));
        reply_data(SENT_TEMP, &b, 1);
        break;
    }
    case GET_STATUS: {
        uint8_t st = 0;
        if (papertest() != 0)                         st |= 0x01;  // 缺纸
        if (get_head_temperature() >= 80.0f)          st |= 0x02;  // 过温
        if (PRINTER_BATTERY <= 10)                    st |= 0x04;  // 低电
        reply_data(SENT_STATUS, &st, 1);
        break;
    }
    case GET_HEAT_DENSITY: {
        uint8_t d = job_get_density();
        reply_data(SENT_HEAT_DENSITY, &d, 1);
        break;
    }
    case GET_POWER_DOWN_TIME: {
        uint8_t b[2] = {(uint8_t)(s_power_down_min & 0xFF), (uint8_t)(s_power_down_min >> 8)};
        reply_data(SENT_POWER_DOWN_TIME, b, 2);
        break;
    }
    case GET_PAPER_TYPE: {
        uint8_t b = 0x00;                                          // 连续纸
        reply_data(SENT_PAPER_TYPE, &b, 1);
        break;
    }
    case GET_BOARD_VERSION: {
        uint8_t b = 0x01;
        reply_data(SENT_BOARD_VERSION, &b, 1);
        break;
    }

    // ---------- 厂商扩展（握手第 3、4 步） ----------
    case CMD_42:                                                   // 问型号
        reply_data(CMD_43, (const uint8_t *)PROTO_CMD_42_DATA, 6); // "BK3432"
        break;
    case CMD_7F: {                                                 // 问固件版本
        static const uint8_t v[12] = {0x76, 0x33, 0x2e, 0x33, 0x38, 0x2e,
                                      0x31, 0x39, 0, 0, 0, 0};      // "v3.38.19"
        reply_data(CMD_80, v, sizeof(v));
        break;
    }
    case CMD_81: {                                                 // 问硬件信息
        static const uint8_t hw[16] = {0x48, 0x4d, 0x45, 0x32, 0x33, 0x30, 0x5f, 0x50,
                                       0x32, 0, 0, 0, 0, 0, 0, 0};  // "HME230_P2"
        reply_data(CMD_82, hw, sizeof(hw));
        break;
    }
    case CMD_40: {                                                 // 默认参数
        static const uint8_t d[1] = {0x00};
        reply_data(CMD_41, d, 1);
        break;
    }

    // ---------- 设置类：回 ACK ----------
    case SET_CRC_KEY:
        if (n >= 4) {
            uint32_t key = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                           ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
            Serial.printf("SET_CRC_KEY 0x%08X\n", (unsigned)key);
        }
        reply_ack(t);
        break;

    case SET_HEAT_DENSITY:
        if (n >= 1) job_set_density(p[0]);
        reply_ack(t);
        break;

    case SET_POWER_DOWN_TIME:
        if (n >= 2) {
            s_power_down_min = (uint16_t)(p[0] | (p[1] << 8));
            Serial.printf("自动关机 %u 分钟\n", (unsigned)s_power_down_min);
        }
        reply_ack(t);
        break;

    case SET_PAPER_TYPE:
        reply_ack(t);
        break;

    // ---------- 打印数据 ----------
    case PRINT_DATA: {
        if (!s_armed) {                       // App 没发"开始"就直接给数据，也认
            s_armed = true;
            job_begin();
        }
        if (!job_append(p, n)) Serial.println("页缓冲不足，丢弃本包");
        reply_ack(t);
        break;
    }

    case FEED_LINE:
        if (s_armed && job_bytes() > 0) {     // 第二次 FEED_LINE → 开打
            s_armed = false;
            reply_ack(t);                     // 先应答，再干活（别让 App 等 6 秒）
            job_print();
        } else if (s_armed) {                 // 空任务，只走纸
            s_armed = false;
            reply_ack(t);
            job_feed_mm(5);
        } else {                              // 第一次 FEED_LINE → 开始一页
            s_armed = true;
            job_begin();
            reply_ack(t);
        }
        break;

    case PRINT_TEST_PAGE: {
        reply_ack(t);
        job_begin();
        job_append((const uint8_t *)bitmap_page, sizeof(bitmap_page));
        job_print();
        break;
    }

    case PRINT_DATA_COMPRESS:                // 压缩数据，未实现
        Serial.println("收到压缩数据包，忽略");
        reply_ack(t);
        break;

    case DISCONNECT_BT:
        reply_ack(t);
        Serial.println("App 请求断开");
        break;

    // ---------- 其他一律 ACK ----------
    default:
        Serial.printf("指令 %u（%u 字节）→ ACK\n", (unsigned)t, (unsigned)n);
        reply_ack(t);
        break;
    }
}

void bt_print_init(void){
    s_parser.reset();
    s_armed = false;
    s_power_down_min = 3600;

    Serial.print("协议自检：");
    Serial.println(frame_selftest(s_parser, s_frame) ? "OK" : "失败");
    Serial.printf("蓝牙设备名：%s\n", BT_NAME);
}

void bt_print_poll(void){
    uint16_t budget = POLL_BYTE_BUDGET;

    while (SerialBT.available() && budget--) {
        int r = s_parser.feed((uint8_t)SerialBT.read(), s_frame);

        if (r != 1) continue;

        if (!s_frame.crc_ok) {
            Serial.printf("CRC 错：type=%u len=%u 收=%08X 算=%08X\n",
                          (unsigned)s_frame.type, (unsigned)s_frame.len,
                          (unsigned)s_frame.crc_recv, (unsigned)s_frame.crc_calc);
            continue;
        }
        handle_frame();
    }
}
