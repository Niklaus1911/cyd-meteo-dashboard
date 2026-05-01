#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "AppState.h"

namespace AppStateStore {

bool begin();
bool copy(AppState& out, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool setWifiStatus(bool connected,
                   const char* ssid,
                   int rssiDbm,
                   TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool setMqttStatus(bool connected,
                   const char* clientId,
                   TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool updateTelemetry(const SensorTelemetry& telemetry,
                     uint32_t updateMs,
                     TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool updateUptime(uint32_t uptimeMs, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

SemaphoreHandle_t mutexHandle();

}  // namespace AppStateStore
