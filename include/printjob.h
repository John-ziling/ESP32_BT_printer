#pragma once
// =====================================================
//  printjob.h —— 一页打印任务的缓冲与执行
//
//  喵喵机 App 的顺序是：先发完所有 PRINT_DATA，再发一条 FEED_LINE 触发打印。
//  所以必须先把整页点阵攒在内存里（1280 行 × 48B ≈ 60KB），
//  收到触发信号后再逐行走纸 + 加热。
// =====================================================
#include <Arduino.h>
#include "config.h"

#define PAGE_MAX_BYTES (64u * 1024u)   // 上限 64KB ≈ 1365 行 ≈ 170mm

bool     job_begin(void);                       // 开一页（清空）；false = 内存不足
void     job_reset(void);                       // 丢弃本页
bool     job_append(const uint8_t *d, uint16_t n);  // 追加点阵字节
uint32_t job_bytes(void);                       // 已攒字节
uint32_t job_lines(void);                       // 已攒行数（整行）
bool     job_print(void);                       // 走纸 + 加热，返回 false = 中途中止
void     job_feed_mm(float mm);                 // 单独走纸
void     job_set_density(uint8_t d);            // 热密度 0-100
uint8_t  job_get_density(void);
