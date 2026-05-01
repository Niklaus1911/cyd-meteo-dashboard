#pragma once

#include <TFT_eSPI.h>

namespace TftDisplay {

void begin();
TFT_eSPI& tft();
void logConfig();

}  // namespace TftDisplay
