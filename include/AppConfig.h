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

constexpr uint32_t NetworkTaskStackBytes = 24576;
constexpr uint32_t UiTaskStackBytes = 12288;

constexpr uint32_t NetworkLoopPeriodMs = 250;
constexpr uint32_t UiLoopPeriodMs = 5;
constexpr uint32_t UiRefreshPeriodMs = 500;
constexpr uint32_t StatusLogPeriodMs = 5000;
constexpr uint32_t DiagnosticLogPeriodMs = 30000;
constexpr uint32_t StackLogPeriodMs = 30000;
constexpr uint32_t RssiUpdatePeriodMs = 10000;
constexpr uint32_t HeapUpdatePeriodMs = 30000;

constexpr uint8_t CommandQueueLength = 8;
constexpr uint8_t MqttInboundQueueLength = 6;

constexpr const char* MqttClientId = "cyd-dashboard";
constexpr const char* MqttAvailabilityTopic = "home/cyd/status";
constexpr const char* WifiManagerPortalSsid = "CYD-Dashboard-Setup";

constexpr const char* EspHomeBatteryTopic = "esp-c3-meteo-v2/sensor/18650_battery_level/state";
constexpr const char* EspHomeOutsideTemperatureTopic = "esp-c3-meteo-v2/sensor/outside_temperature/state";
constexpr const char* EspHomeSolarPanelVoltageTopic = "esp-c3-meteo-v2/sensor/solar_raw_voltage/state";
constexpr const char* EspHomeSolarPanelCurrentTopic = "esp-c3-meteo-v2/sensor/solar_panel_current/state";
constexpr const char* EspHomeOutsideHumidityTopic = "esp-c3-meteo-v2/sensor/outside_humidity/state";
constexpr const char* EspHomeAbsolutePressureTopic = "esp-c3-meteo-v2/sensor/absolute_pressure/state";

constexpr const char* ForecastRegionTopic = "home/cyd/zambretti/region";
constexpr const char* ForecastAlertTopic = "home/cyd/zambretti/alert";
constexpr const char* ForecastTextTopic = "home/cyd/zambretti/forecast";
constexpr const char* ForecastLowSummaryTopic = "home/cyd/zambretti/low_summary";
constexpr const char* ForecastUpdatedTopic = "home/cyd/zambretti/updated";

constexpr uint32_t SensorExpectedUpdateIntervalMs = 10UL * 60UL * 1000UL;

constexpr uint32_t WifiReconnectIntervalMs = 10000;
constexpr uint32_t MqttReconnectIntervalMs = 5000;
constexpr uint32_t TelemetryStaleAfterMs = SensorExpectedUpdateIntervalMs + (5UL * 60UL * 1000UL);
constexpr uint32_t WifiManagerPortalTimeoutSec = 180;
constexpr uint32_t MqttSocketTimeoutSec = 2;
constexpr uint32_t MqttKeepAliveSec = 30;

// Placeholder: set this to the real CYD button/touch GPIO once the board input is chosen.
constexpr int ConfigResetButtonPin = -1;
constexpr bool ConfigResetButtonActiveLow = true;
constexpr uint32_t ConfigResetHoldMs = 5000;
constexpr uint32_t ConfigResetSampleMs = 50;
constexpr uint32_t RestartAfterConfigResetMs = 1000;

constexpr size_t MqttTopicMaxLength = 96;
constexpr size_t MqttPayloadMaxLength = 512;
constexpr size_t MqttHostMaxLength = 64;
constexpr size_t MqttUserMaxLength = 48;
constexpr size_t MqttPasswordMaxLength = 64;
constexpr uint16_t DefaultMqttPort = 1883;

constexpr size_t ForecastRegionMaxLength = 48;
constexpr size_t ForecastAlertMaxLength = 96;
constexpr size_t ForecastTextMaxLength = 256;
constexpr size_t ForecastLowSummaryMaxLength = 384;
constexpr size_t ForecastUpdatedMaxLength = 24;

}  // namespace AppConfig
