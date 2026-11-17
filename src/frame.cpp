#include <Arduino.h>
#include <string.h>

#include "frame.h"
#include "crc32.h"

// =====================================================
//  frame.cpp —— 喵喵机协议的组帧 / 拆帧
// =====================================================

// ACK 形状的帧：载荷长度 1 且内容 0x00
static inline bool ack_shaped(uint16_t len, const uint8_t *payload){
    return (len == 1 && payload[0] == 0x00);
}

void FrameParser::reset(){
    st = S_START;
    type = 0; index = 0; len = 0; pos = 0; crc = 0; crc_pos = 0;
}

int FrameParser::feed(uint8_t b, Frame &out){
    switch (st) {
    case S_START:
        if (b == FRAME_START) { st = S_TYPE; return 0; }
        bad_cnt++;              // 杂散字节（比如上一帧没对齐），丢弃继续找帧头
        return -1;

    case S_TYPE:  type = b;  st = S_INDEX; return 0;
    case S_INDEX: index = b; st = S_LENL;  return 0;
    case S_LENL:  len = b;   st = S_LENH;  return 0;

    case S_LENH:
        len |= (uint16_t)b << 8;
        if (len > FRAME_MAX_PAYLOAD) { bad_cnt++; reset(); return -1; }
        pos = 0;
        if (len == 0) { crc = 0; crc_pos = 0; st = S_CRC; }
        else          { st = S_DATA; }
        return 0;

    case S_DATA:
        buf[pos++] = b;
        if (pos >= len) { crc = 0; crc_pos = 0; st = S_CRC; }
        return 0;

    case S_CRC:
        crc |= (uint32_t)b << (8 * crc_pos);
        if (++crc_pos >= 4) st = S_END;
        return 0;

    case S_END: {
        if (b != FRAME_END) { bad_cnt++; reset(); return -1; }

        out.type  = type;
        out.index = index;
        out.len   = len;
        if (len) memcpy(out.payload, buf, len);
        out.crc_recv = crc;

        if (len == 0) {
            // 载荷为空的帧（所有 GET 类指令）：实测两种写法都存在——
            // 有的实现算空载荷（CRC=0），有的算帧头 5 字节，两者都放行
            uint8_t head[FRAME_HDR_LEN] = {
                FRAME_START, type, index, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8)
            };
            out.crc_calc = crc32_calc(head, FRAME_HDR_LEN);
            out.crc_ok   = (crc == 0 || crc == out.crc_calc);
        } else if (ack_shaped(len, buf)) {
            // ACK：CRC 覆盖帧头 5 字节 + 那 1 字节 0x00 载荷
            uint8_t head[FRAME_HDR_LEN + 1] = {
                FRAME_START, type, index,
                (uint8_t)(len & 0xFF), (uint8_t)(len >> 8), buf[0]
            };
            out.crc_calc = crc32_calc(head, sizeof(head));
            out.crc_ok   = (out.crc_calc == crc);
        } else {
            out.crc_calc = crc32_calc(buf, len);
            out.crc_ok   = (out.crc_calc == crc);
        }

        if (out.crc_ok) ok_cnt++; else bad_cnt++;

        reset();
        return 1;
    }
    }
    return -1;
}

uint16_t frame_build(uint8_t type, uint8_t index, const uint8_t *payload,
                     uint16_t len, uint8_t *out, uint16_t cap){
    if (len > FRAME_MAX_PAYLOAD) return 0;
    uint16_t total = FRAME_HDR_LEN + len + FRAME_TAIL_LEN;
    if (cap < total) return 0;

    out[0] = FRAME_START;
    out[1] = type;
    out[2] = index;
    out[3] = (uint8_t)(len & 0xFF);
    out[4] = (uint8_t)(len >> 8);
    if (len) memcpy(out + FRAME_HDR_LEN, payload, len);

    uint32_t c = crc32_calc(out + FRAME_HDR_LEN, len);
    uint8_t *p = out + FRAME_HDR_LEN + len;
    p[0] = (uint8_t)(c);
    p[1] = (uint8_t)(c >> 8);
    p[2] = (uint8_t)(c >> 16);
    p[3] = (uint8_t)(c >> 24);

    out[total - 1] = FRAME_END;
    return total;
}

uint16_t ack_build(uint8_t type, uint8_t *out){
    out[0] = FRAME_START;
    out[1] = type;
    out[2] = 0x00;   // 包序号，设备侧固定 0
    out[3] = 0x01;   // 长度 1（小端）
    out[4] = 0x00;
    out[5] = 0x00;   // 载荷固定一个 0x00，表示"收到且成功"

    uint32_t c = crc32_calc(out, FRAME_HDR_LEN + 1);
    out[6] = (uint8_t)(c);
    out[7] = (uint8_t)(c >> 8);
    out[8] = (uint8_t)(c >> 16);
    out[9] = (uint8_t)(c >> 24);
    out[10] = FRAME_END;
    return ACK_LEN;
}

void frame_hexdump(const char *tag, const uint8_t *d, size_t n){
    Serial.printf("%s (%u):", tag, (unsigned)n);
    for (size_t i = 0; i < n; i++) {
        if (i % 16 == 0) Serial.print("\n  ");
        Serial.printf("%02X ", d[i]);
    }
    Serial.println();
}

bool frame_selftest(FrameParser &parser, Frame &frame){
    bool ok = true;

    // 1) CRC 标准向量
    if (crc32_calc((const uint8_t *)"123456789", 9) != 0xCBF43926u) {
        Serial.println("  [x] CRC 标准向量 123456789"); ok = false;
    }
    if (crc32_calc((const uint8_t *)"P1001705253855", 14) != 0xBEFC12DFu) {
        Serial.println("  [x] CRC SN 向量"); ok = false;
    }
    const uint8_t ver_payload[3] = {0x01, 0x00, 0x02};
    if (crc32_calc(ver_payload, 3) != 0x108DD209u) {
        Serial.println("  [x] CRC 版本号向量"); ok = false;
    }

    // 2) ACK 字节序（GET_VERSION 应答应为 02 04 00 01 00 00 5F CC 48 08 03）
    static const uint8_t ack_expect[ACK_LEN] = {
        0x02, 0x04, 0x00, 0x01, 0x00, 0x00, 0x5F, 0xCC, 0x48, 0x08, 0x03
    };
    uint8_t ack[ACK_LEN];
    ack_build(4, ack);
    if (memcmp(ack, ack_expect, ACK_LEN) != 0) {
        Serial.println("  [x] ACK 组帧");
        frame_hexdump("    得到", ack, ACK_LEN);
        ok = false;
    }

    // 3) 组帧 → 拆帧 往返（数据帧）
    uint8_t raw[64];
    uint16_t n = frame_build(5, 0, ver_payload, 3, raw, sizeof(raw));
    if (n != 13) { Serial.println("  [x] 数据帧长度"); ok = false; }

    parser.reset();
    int r = 0;
    for (uint16_t i = 0; i < n; i++) {
        r = parser.feed(raw[i], frame);
        if (r != 0 && i != n - 1) break;
    }
    if (r != 1 || !frame.crc_ok || frame.type != 5 || frame.len != 3 ||
        memcmp(frame.payload, ver_payload, 3) != 0) {
        Serial.println("  [x] 数据帧往返"); ok = false;
    }

    // 4) 拆 ACK（CRC 覆盖前 6 字节的分支）
    parser.reset();
    r = 0;
    for (uint16_t i = 0; i < ACK_LEN; i++) r = parser.feed(ack[i], frame);
    if (r != 1 || !frame.crc_ok || frame.type != 4) {
        Serial.println("  [x] ACK 拆帧"); ok = false;
    }

    // 5) 载荷为空的帧（GET 类指令），CRC 覆盖帧头 5 字节
    uint8_t empty_hdr[FRAME_HDR_LEN] = {FRAME_START, 4, 0x00, 0x00, 0x00};
    uint32_t hc = crc32_calc(empty_hdr, FRAME_HDR_LEN);
    uint8_t empty_frame[10] = {FRAME_START, 4, 0x00, 0x00, 0x00,
                               (uint8_t)hc, (uint8_t)(hc >> 8), (uint8_t)(hc >> 16), (uint8_t)(hc >> 24),
                               FRAME_END};
    parser.reset();
    r = 0;
    for (uint16_t i = 0; i < sizeof(empty_frame); i++) r = parser.feed(empty_frame[i], frame);
    if (r != 1 || !frame.crc_ok || frame.type != 4 || frame.len != 0) {
        Serial.println("  [x] 空载荷帧"); ok = false;
    }

    return ok;
}
