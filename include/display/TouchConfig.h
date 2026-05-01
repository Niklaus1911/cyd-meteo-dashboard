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

constexpr int32_t RawMinX = 200;
constexpr int32_t RawMaxX = 3700;
constexpr int32_t RawMinY = 240;
constexpr int32_t RawMaxY = 3800;
constexpr bool SwapXY = true;
constexpr bool InvertX = true;
constexpr bool InvertY = false;

constexpr bool DebugLogTouches = false;
constexpr uint32_t DebugLogPeriodMs = 250;

}  // namespace TouchConfig
