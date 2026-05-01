#pragma once

#include <Arduino.h>
#include <cstring>

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
constexpr uint8_t MqttInboundQueueLength = 6;

constexpr const char* MqttClientId = "cyd-dashboard";

constexpr const char* WifiSsid = "[PUT WIFI SSID HERE]";
constexpr const char* WifiPassword = "[PUT WIFI PASSWORD HERE]";

constexpr const char* MqttHost = "[PUT BROKER IP/HOST HERE]";
constexpr uint16_t MqttPort = 1883;
constexpr const char* MqttUsername = "[PUT USERNAME OR \"none\"]";
constexpr const char* MqttPassword = "[PUT PASSWORD OR \"none\"]";

constexpr const char* MqttTelemetryTopic = "[PUT SENSOR TOPIC HERE]";
constexpr const char* MqttAvailabilityTopic = "home/cyd/status";

constexpr uint32_t WifiReconnectIntervalMs = 10000;
constexpr uint32_t MqttReconnectIntervalMs = 5000;
constexpr uint32_t TelemetryStaleAfterMs = 60000;
constexpr uint32_t MqttSocketTimeoutSec = 2;
constexpr uint32_t MqttKeepAliveSec = 30;

constexpr size_t MqttTopicMaxLength = 96;
constexpr size_t MqttPayloadMaxLength = 512;

inline bool mqttCredentialsConfigured() {
  return MqttUsername[0] != '\0' && strcmp(MqttUsername, "none") != 0 &&
         strcmp(MqttUsername, "[PUT USERNAME OR \"none\"]") != 0;
}

}  // namespace AppConfig
