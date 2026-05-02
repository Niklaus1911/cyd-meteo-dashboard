#pragma once

#include <Arduino.h>

struct TouchCalibrationData {
  int32_t rawMinX = 0;
  int32_t rawMaxX = 0;
  int32_t rawMinY = 0;
  int32_t rawMaxY = 0;
  int16_t offsetX = 0;
  int16_t offsetY = 0;
  bool swapXY = false;
  bool invertX = false;
  bool invertY = false;
};

namespace TouchCalibrationStore {

bool load(TouchCalibrationData& calibration);
bool save(const TouchCalibrationData& calibration);
bool clear();

}  // namespace TouchCalibrationStore
