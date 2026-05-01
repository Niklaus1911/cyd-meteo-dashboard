#pragma once

#include <Arduino.h>
#include <SPI.h>

namespace TouchConfig {

constexpr bool Enabled = true;

constexpr int CsPin = 33;
constexpr int IrqPin = 36;
constexpr bool UseIrqPin = true;

constexpr uint8_t SpiBus = HSPI;
constexpr int SclkPin = 25;
constexpr int MisoPin = 39;
constexpr int MosiPin = 32;

// CYD landscape rotation 1 calibration.
// Recalibrate by enabling DebugLogTouches, touching the four visible corners,
// then setting X min/max from the left/right raw X values and Y min/max from
// the top/bottom raw Y values. For this panel, raw X increases left-to-right
// and raw Y increases top-to-bottom, so axes are not swapped or inverted.
constexpr int32_t RawMinX = 289;
constexpr int32_t RawMaxX = 3605;
constexpr int32_t RawMinY = 562;
constexpr int32_t RawMaxY = 3641;
constexpr bool SwapXY = false;
constexpr bool InvertX = false;
constexpr bool InvertY = false;

constexpr bool DebugLogTouches = false;
constexpr uint32_t DebugLogPeriodMs = 250;

}  // namespace TouchConfig
