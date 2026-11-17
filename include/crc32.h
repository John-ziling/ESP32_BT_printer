#pragma once
// =====================================================
//  crc32.h —— 标准 CRC-32（zlib / PNG 用的同一种）
//  多项式 0xEDB88320（反射），初值 0xFFFFFFFF，末尾取反
// =====================================================
#include <stdint.h>
#include <stddef.h>

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);  // 分段累加
uint32_t crc32_calc(const uint8_t *data, size_t len);                  // 一次算完（标准 zlib）

// 喵喵机协议实际用的变体：用 crcKey 代替 0xFFFFFFFF 作初值
uint32_t crc32_seeded(uint32_t seed, const uint8_t *data, size_t len);
