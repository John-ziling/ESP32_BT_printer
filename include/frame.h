#pragma once
// =====================================================
//  frame.h —— 喵喵机上位机协议的一帧
//
//   0      1      2      3      4      5..         n-4..n-1   n
//  +------+------+------+------+------+------------+---------+------+
//  | 0x02 | type | idx  | lenL | lenH |  payload   |  CRC32  | 0x03 |
//  +------+------+------+------+------+------------+---------+------+
//
//  长度 2 字节小端，CRC32 4 字节小端。
//  CRC 覆盖范围按帧的类型区分（实测结论，见教程 8.2）：
//    · 数据帧        —— 只算 payload
//    · ACK 帧        —— 算帧头 5 字节 + 那 1 字节 0x00 载荷 = 前 6 字节
//  两者用"载荷长度 1 且内容为 0x00"来区分。
// =====================================================
#include <stdint.h>
#include <stddef.h>

#define FRAME_START       0x02
#define FRAME_END         0x03
#define FRAME_HDR_LEN     5      // 02 + type + idx + lenL + lenH
#define FRAME_TAIL_LEN    5      // 4 字节 CRC + 03
#define FRAME_MAX_PAYLOAD 2048   // 协议规定单包 ≤1920/2016，这里留 2KB
#define ACK_LEN           11

struct Frame {
    uint8_t  type;
    uint8_t  index;
    uint16_t len;
    uint8_t  payload[FRAME_MAX_PAYLOAD];
    uint32_t crc_recv;   // 帧里带的 CRC
    uint32_t crc_calc;   // 本地算出来的 CRC
    bool     crc_ok;
};

class FrameParser {
public:
    void reset();
    // 喂一个字节：1 = 收到完整帧（已填进 out），0 = 继续等，-1 = 错帧（已自动复位）
    int feed(uint8_t b, Frame &out);

    uint32_t ok_cnt  = 0;   // 累计正确帧
    uint32_t bad_cnt = 0;   // 累计错帧/杂散字节

private:
    enum St : uint8_t { S_START, S_TYPE, S_INDEX, S_LENL, S_LENH, S_DATA, S_CRC, S_END };
    St       st       = S_START;
    uint8_t  type     = 0;
    uint8_t  index    = 0;
    uint16_t len      = 0;
    uint16_t pos      = 0;
    uint32_t crc      = 0;
    uint8_t  crc_pos  = 0;
    uint8_t  buf[FRAME_MAX_PAYLOAD];
};

// 组一个数据帧（CRC 覆盖 payload），返回整帧字节数；cap 不够返回 0
uint16_t frame_build(uint8_t type, uint8_t index, const uint8_t *payload,
                     uint16_t len, uint8_t *out, uint16_t cap);
// 组一个 ACK（固定 11 字节，CRC 覆盖前 6 字节），返回 11
uint16_t ack_build(uint8_t type, uint8_t *out);
// 启动自检：CRC 向量 + ACK 字节序 + 组帧/拆帧往返，全过返回 true
// 传入调用方的解析器/帧对象，避免再占 4KB RAM
bool frame_selftest(FrameParser &parser, Frame &frame);
// 十六进制打印，调试用
void frame_hexdump(const char *tag, const uint8_t *d, size_t n);
