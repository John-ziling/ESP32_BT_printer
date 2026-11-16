#include "config.h"

void LEDInit(void) {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void BTNInit(void) {
  pinMode(BTN_PIN, INPUT_PULLUP);
}
