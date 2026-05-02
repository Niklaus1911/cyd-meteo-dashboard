#pragma once

#include <Arduino.h>

namespace DisplayConfig {

// CYD 2USB likely uses GPIO 21 for the TFT backlight. Change here if your board differs.
constexpr int BacklightPin = 21;
constexpr bool BacklightActiveHigh = true;

constexpr uint8_t NormalDisplayRotation = 1;
constexpr uint8_t FlippedDisplayRotation = 3;

// If the orientation is wrong, change this value through 0, 1, 2, 3 and rebuild.
constexpr uint8_t Rotation = NormalDisplayRotation;

// If colors look like a photo negative, toggle this value and rebuild.
constexpr bool InvertColors = true;

// If runtime inversion is not enough on the 2USB panel, also try adding/removing
// -DTFT_INVERSION_ON in platformio.ini build_flags.

inline const char* driverName() {
#if defined(ILI9341_2_DRIVER)
  return "ILI9341_2_DRIVER";
#elif defined(ILI9341_DRIVER)
  return "ILI9341_DRIVER";
#else
  return "UNKNOWN_DRIVER";
#endif
}

inline const char* rgbOrderName() {
#if defined(CYD_TFT_RGB_ORDER_LABEL)
  return CYD_TFT_RGB_ORDER_LABEL;
#else
  return "TFT_RGB_ORDER unset";
#endif
}

inline const char* inversionName() {
  return InvertColors ? "ON" : "OFF";
}

}  // namespace DisplayConfig
