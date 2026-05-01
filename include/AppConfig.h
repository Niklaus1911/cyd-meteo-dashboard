#pragma once

#include <Arduino.h>

namespace AppConfig {

constexpr uint32_t SerialBaud = 115200;

constexpr const char* NetworkTaskName = "NetworkTask";
constexpr const char* UiTaskName = "UiTask";

constexpr BaseType_t NetworkTaskCore = 0;
constexpr BaseType_t UiTaskCore = 1;

constexpr UBaseType_t NetworkTaskPriority = 2;
constexpr UBaseType_t UiTaskPriority = 2;

constexpr uint32_t NetworkTaskStackWords = 4096;
constexpr uint32_t UiTaskStackWords = 4096;

constexpr uint32_t NetworkLoopPeriodMs = 250;
constexpr uint32_t UiLoopPeriodMs = 100;
constexpr uint32_t StatusLogPeriodMs = 5000;

constexpr uint8_t CommandQueueLength = 8;

constexpr const char* MqttClientId = "cyd-dashboard";

}  // namespace AppConfig
