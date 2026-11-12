#pragma once
#include <main.h>

#define HEAT_MAX_TIME_US 3000 
#define HEAT_DEF_TIME_US 800 


static float feed_accum = 0.0f;

void printinit(void);
void printhead_vh(bool on);
void printhead_send(const uint8_t row[HEAD_BYTES]);
void printer_clear(void);
void printhead_stb(uint8_t mask);
void printhead_heat_mask(uint8_t mask,uint32_t time_us);
void printhead_heat_all(uint32_t time_us);
void printer_print_row(const uint8_t row[HEAD_BYTES], uint32_t heat_time_us);

