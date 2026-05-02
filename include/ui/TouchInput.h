#pragma once

#include <stdint.h>

#include "app/TouchCalibration.h"

namespace TouchInput {

struct RawPoint {
  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
};

void begin(uint16_t screenWidth, uint16_t screenHeight);
void resetState();
void ignoreInputFor(uint32_t durationMs);
bool isInputIgnored();
bool isPressed();
bool readRawPoint(RawPoint& point);
bool saveCalibration(const TouchCalibrationData& calibration);
bool isCalibrationSaved();
const TouchCalibrationData& calibration();

}  // namespace TouchInput
