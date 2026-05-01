#pragma once

#include <Arduino.h>

#include "AppConfig.h"

enum class SensorField : uint8_t {
  BatteryPercent,
  OutsideTemperatureC,
  SolarPanelVoltageV,
  SolarPanelCurrentmA,
  OutsideHumidityPercent,
  AbsolutePressurehPa,
};

enum class ForecastField : uint8_t {
  Region,
  Alert,
  Text,
  LowSummary,
  Updated,
};

struct SensorTelemetry {
  bool valid = false;
  bool stale = true;

  float batteryPercent = NAN;
  bool batteryPercentValid = false;

  float outsideTemperatureC = NAN;
  bool outsideTemperatureValid = false;

  float solarPanelVoltageV = NAN;
  bool solarPanelVoltageValid = false;

  float solarPanelCurrentmA = NAN;
  bool solarPanelCurrentValid = false;

  float outsideHumidityPercent = NAN;
  bool outsideHumidityValid = false;

  float absolutePressurehPa = NAN;
  bool absolutePressureValid = false;
};

struct AppState {
  bool wifiConnected = false;
  char wifiSsid[33] = {};
  char wifiIpAddress[16] = {};
  int wifiRssiDbm = 0;

  bool mqttConnected = false;
  char mqttClientId[32] = {};
  char mqttBrokerHost[64] = {};
  bool credentialResetRequested = false;
  bool credentialResetting = false;
  bool credentialRebooting = false;

  SensorTelemetry latestSensor = {};
  uint32_t lastTelemetryUpdateMs = 0;
  uint32_t lastMqttReceiveMs = 0;
  uint32_t uptimeMs = 0;
  uint32_t freeHeapBytes = 0;

  bool forecastValid = false;
  char forecastRegion[AppConfig::ForecastRegionMaxLength] = {};
  char forecastAlert[AppConfig::ForecastAlertMaxLength] = {};
  char forecastText[AppConfig::ForecastTextMaxLength] = {};
  char forecastLowSummary[AppConfig::ForecastLowSummaryMaxLength] = {};
  char forecastUpdated[AppConfig::ForecastUpdatedMaxLength] = {};
  uint32_t forecastLastReceiveMs = 0;
};
