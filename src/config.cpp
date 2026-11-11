#include "config.h"

void LEDconfig() {
  pinMode(LED_PIN, OUTPUT);
}
void BTNconfig() {
  pinMode(BTN_PIN, INPUT_PULLUP);
}
