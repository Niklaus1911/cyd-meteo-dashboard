#pragma once

#include <TFT_eSPI.h>

namespace TftDisplay {

void begin();
TFT_eSPI& tft();
void logConfig();
uint8_t rotation();
bool isFlipped180();

}  // namespace TftDisplay
