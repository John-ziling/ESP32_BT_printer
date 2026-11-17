#include <Arduino.h>
#include <string.h>

#include "frame.h"
#include "crc32.h"
#include "proto.h"

// =====================================================
//  frame.cpp —— 喵喵机协议的组帧 / 拆帧
//
//  CRC 规则（以真机抓包为准，不是书上的说法）：
//    crc = crc32_seeded(crcKey, payload, len)
//    即把 crcKey（0x35769521）代替 0xFFFFFFFF 作初值，且**只覆盖 payload**。
//    实盘证据：App 的 SET_CRC_KEY 帧 payload=21 95 76 35，帧内 CRC=0x2144DF1C
//             = crc32_seeded(0x35769521, 21 95 76 35)
//    推论：空载荷帧的 CRC 恒等于 key 本身（0x35769521）。
//
//  为了兼容早期按"标准 zlib / 覆盖帧头"写的上位机，校验时几种候选都试一遍，
//  命中哪个就记在 Frame::crc_rule 里。
// =====================================================

// ACK 形状的帧：载荷长度 1 且内容 0x00

static inline bool ack_shaped(uint16_t len, const uint8_t *payload){
    return (len == 1 && payload[0] == 0x00);
}

static uint32_t s_crc_key = FRAME_CRC_KEY_DEFAULT;

void frame_set_crc_key(uint32_t key){ s_crc_key = key; }
uint32_t frame_get_crc_key(void){ return s_crc_key; }

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

        // 帧头 5 字节；ACK 形状再加那 1 字节载荷
        uint8_t  head[FRAME_HDR_LEN + 1] = {
            FRAME_START, type, index, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8), 0x00
        };
        uint16_t head_len = FRAME_HDR_LEN;
        if (ack_shaped(len, buf)) { head[FRAME_HDR_LEN] = buf[0]; head_len++; }

        uint32_t cand[5];
        uint8_t  cand_n = 0;

        cand[cand_n++] = crc32_seeded(s_crc_key, buf, len);   // 0: 主规则（App 实测）
        cand[cand_n++] = crc32_calc(buf, len);                // 1: 标准 zlib，覆盖 payload
        cand[cand_n++] = crc32_calc(head, head_len);          // 2: 标准 zlib，覆盖帧头
        cand[cand_n++] = crc32_seeded(s_crc_key, head, head_len); // 3: key 初值，覆盖帧头

        if (type == SET_CRC_KEY && len == 4) {                // 4: 用帧里带的 key 校验自己
            uint32_t k = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
                         ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
            cand[cand_n++] = crc32_seeded(k, buf, len);
        }

        out.crc_calc = cand[0];
        out.crc_ok   = false;
        out.crc_rule = 0xFF;
        for (uint8_t i = 0; i < cand_n; i++) {
            if (cand[i] == crc) { out.crc_ok = true; out.crc_rule = i; break; }
        }

        // 老工具发的全 0 CRC（空载荷帧）也放行
        if (!out.crc_ok && len == 0 && crc == 0) { out.crc_ok = true; out.crc_rule = 5; }

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

    uint32_t c = crc32_seeded(s_crc_key, out + FRAME_HDR_LEN, len);
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

    // 只覆盖那 1 字节载荷（厂商 send_ack 就是这么算的）
    uint32_t c = crc32_seeded(s_crc_key, out + FRAME_HDR_LEN, 1);
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

    // 1) 标准 zlib 向量（保留，用于确认表没坏）
    if (crc32_calc((const uint8_t *)"123456789", 9) != 0xCBF43926u) {
        Serial.println("  [x] CRC 标准向量 123456789"); ok = false;
    }

    // 2) key 作初值的向量（真机实测值）
    const uint8_t key_payload[4] = {0x21, 0x95, 0x76, 0x35};
    if (crc32_seeded(FRAME_CRC_KEY_DEFAULT, key_payload, 4) != 0x2144DF1Cu) {
        Serial.println("  [x] CRC key 向量（真机样本）"); ok = false;
    }
    if (crc32_seeded(FRAME_CRC_KEY_DEFAULT, nullptr, 0) != FRAME_CRC_KEY_DEFAULT) {
        Serial.println("  [x] 空载荷 CRC 应等于 key"); ok = false;
    }
    const uint8_t ver_payload[3] = {0x01, 0x00, 0x02};
    if (crc32_seeded(FRAME_CRC_KEY_DEFAULT, ver_payload, 3) != 0x9C5254BDu) {
        Serial.println("  [x] 版本号 key 向量"); ok = false;
    }
    if (crc32_seeded(FRAME_CRC_KEY_DEFAULT, (const uint8_t *)"P1001705253855", 14) != 0x759F78E5u) {
        Serial.println("  [x] SN key 向量"); ok = false;
    }

    // 3) ACK 字节序：GET_VERSION 应答应为 02 04 00 01 00 00 46 89 5E 9E 03
    static const uint8_t ack_expect[ACK_LEN] = {
        0x02, 0x04, 0x00, 0x01, 0x00, 0x00, 0x46, 0x89, 0x5E, 0x9E, 0x03
    };
    uint8_t ack[ACK_LEN];
    ack_build(4, ack);
    if (memcmp(ack, ack_expect, ACK_LEN) != 0) {
        Serial.println("  [x] ACK 组帧");
        frame_hexdump("    得到", ack, ACK_LEN);
        ok = false;
    }

    // 4) 组帧 → 拆帧 往返（数据帧）
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

    // 5) 拆 ACK
    parser.reset();
    r = 0;
    for (uint16_t i = 0; i < ACK_LEN; i++) r = parser.feed(ack[i], frame);
    if (r != 1 || !frame.crc_ok || frame.type != 4) {
        Serial.println("  [x] ACK 拆帧"); ok = false;
    }

    // 6) App 真实的 GET_VERSION 帧：02 04 00 00 00 21 95 76 35 03
    static const uint8_t app_empty[10] = {
        0x02, 0x04, 0x00, 0x00, 0x00, 0x21, 0x95, 0x76, 0x35, 0x03
    };
    parser.reset();
    r = 0;
    for (uint16_t i = 0; i < sizeof(app_empty); i++) r = parser.feed(app_empty[i], frame);
    if (r != 1 || !frame.crc_ok || frame.type != 4 || frame.len != 0) {
        Serial.println("  [x] App 空载荷帧"); ok = false;
    }

    // 7) App 真实的 SET_CRC_KEY 帧（CRC 由帧内的 key 算出）
    uint8_t key_frame[64];
    frame_set_crc_key(0x11111111u);            // 故意用一个错 key 组帧
    uint16_t klen = frame_build(SET_CRC_KEY, 0, key_payload, 4, key_frame, sizeof(key_frame));
    // 把 CRC 换成真机那个值，确认"载荷里的 key 自校验"这条规则能命中
    key_frame[klen - 5] = 0x1C; key_frame[klen - 4] = 0xDF;
    key_frame[klen - 3] = 0x44; key_frame[klen - 2] = 0x21;
    parser.reset();
    r = 0;
    for (uint16_t i = 0; i < klen; i++) r = parser.feed(key_frame[i], frame);
    if (r != 1 || !frame.crc_ok) {
        Serial.println("  [x] SET_CRC_KEY 自校验帧"); ok = false;
    }
    frame_set_crc_key(FRAME_CRC_KEY_DEFAULT);

    return ok;
}
