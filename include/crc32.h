#pragma once
// =====================================================
//  crc32.h —— 标准 CRC-32（zlib / PNG 用的同一种）
//  多项式 0xEDB88320（反射），初值 0xFFFFFFFF，末尾取反
// =====================================================
#include <stdint.h>
#include <stddef.h>

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);  // 分段累加
uint32_t crc32_calc(const uint8_t *data, size_t len);                  // 一次算完
