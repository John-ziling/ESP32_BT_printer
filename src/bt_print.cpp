#include "bt_print.h"
#include "frame.h"
#include "proto.h"

// =====================================================
//  bt_print.cpp —— 上位机（喵喵机 App）通信
//
//  收：0x02 | type | idx | len(2) | payload | CRC32 | 0x03
//  回：每条有效指令都回 11 字节 ACK（CRC 覆盖前 6 字节）
//  CRC 错的帧不回 ACK —— 让 App 超时重发，避免把坏数据当成功
//
//  当前阶段：只做拆帧 + CRC 校验 + ACK + 指令分发骨架。
//  握手应答（SENT_VERSION / SENT_SN …）在第 3 步补，
//  打印数据通路（PRINT_DATA → 48 字节行 → 加热）在第 4 步补。
// =====================================================

#define POLL_BYTE_BUDGET 512   // 单次 poll 最多吃多少字节，别把 loop() 饿住

static FrameParser s_parser;
static Frame       s_frame;

// 蓝牙发送：分块写 + 重试。
// 注意不要用 availableForWrite() —— 这一版 BluetoothSerial 没有实现它，
// 基类 Print 永远返回 0，照教程那样写会死循环。
static bool bt_send(const uint8_t *buf, size_t len){
    size_t sent = 0;
    uint16_t retry = 0;

    while (sent < len) {
        size_t chunk = len - sent;
        if (chunk > 512) chunk = 512;

        size_t n = SerialBT.write(buf + sent, chunk);
        sent += n;

        if (n < chunk) {
            if (!SerialBT.hasClient()) return false;   // 对端已断开
            if (++retry > 300) return false;           // ~300ms 还写不动就放弃
            delay(1);
        } else {
            retry = 0;
        }
    }
    return true;
}

static void send_ack(uint8_t type){
    uint8_t buf[ACK_LEN];
    ack_build(type, buf);
    bt_send(buf, ACK_LEN);
}

static void handle_frame(void){
    const uint8_t *p   = s_frame.payload;
    uint16_t       n   = s_frame.len;
    uint8_t        type = s_frame.type;

    switch (type) {
    case SET_CRC_KEY:
        if (n >= 4) {
            uint32_t key = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                           ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
            Serial.printf("SET_CRC_KEY = 0x%08X\n", (unsigned)key);
        }
        break;

    case PRINT_DATA:
        // 第 4 步：把 payload 喂给打印数据通路
        Serial.printf("PRINT_DATA %u 字节（通路未接）\n", (unsigned)n);
        break;

    case FEED_LINE:
        Serial.println("FEED_LINE");
        break;

    default:
        Serial.printf("指令 %u（%u 字节）未实现\n", (unsigned)type, (unsigned)n);
        break;
    }
}

void bt_print_init(void){
    s_parser.reset();
    Serial.print("协议自检：");
    Serial.println(frame_selftest(s_parser, s_frame) ? "OK" : "失败");
    Serial.printf("蓝牙设备名：%s\n", BT_NAME);
}

// 非阻塞轮询：吃字节 → 拆帧 → 校验 → 应答 → 分发
void bt_print_poll(void){
    uint16_t budget = POLL_BYTE_BUDGET;

    while (SerialBT.available() && budget--) {
        int r = s_parser.feed((uint8_t)SerialBT.read(), s_frame);

        if (r < 0) continue;          // 杂散字节 / 错帧，解析器已复位
        if (r != 1) continue;         // 还没凑齐一帧

        if (!s_frame.crc_ok) {
            Serial.printf("CRC 错：type=%u len=%u 收=%08X 算=%08X（不回应答）\n",
                          (unsigned)s_frame.type, (unsigned)s_frame.len,
                          (unsigned)s_frame.crc_recv, (unsigned)s_frame.crc_calc);
            continue;
        }

        send_ack(s_frame.type);       // 先应答，再干活
        handle_frame();
    }
}
