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

bool setWifiIpAddress(const char* ipAddress, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool setMqttStatus(bool connected,
                   const char* clientId,
                   TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool setMqttBrokerHost(const char* host, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool setCredentialResetStatus(bool requested,
                              bool resetting,
                              bool rebooting,
                              TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool updateTelemetry(const SensorTelemetry& telemetry,
                     uint32_t updateMs,
                     TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool updateSensorValue(SensorField field,
                       float value,
                       uint32_t updateMs,
                       TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool updateForecastValue(ForecastField field,
                         const char* value,
                         uint32_t updateMs,
                         TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool setTelemetryStale(bool stale, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool recordMqttReceive(uint32_t receiveMs, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool updateUptime(uint32_t uptimeMs, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool updateFreeHeap(uint32_t freeHeapBytes, TickType_t timeoutTicks = pdMS_TO_TICKS(20));

bool setDisplayOrientation(bool flipped180,
                           uint8_t rotation,
                           TickType_t timeoutTicks = pdMS_TO_TICKS(20));

SemaphoreHandle_t mutexHandle();

}  // namespace AppStateStore
