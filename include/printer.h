#pragma once
#include <main.h>

void printinit(void);
void printhead_vh(bool on);
void printhead_send(const uint8_t row[HEAD_BYTES]);
void printer_clear();
void printhead_stb(uint8_t mask);
void printhead_heat_mask(uint8_t mask,uint32_t time_us);
void printhead_heat_all(uint32_t time_us);

