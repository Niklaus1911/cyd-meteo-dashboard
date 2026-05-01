#pragma once

#include <Arduino.h>

enum class SensorField : uint8_t {
  BatteryPercent,
  OutsideTemperatureC,
  SolarPanelVoltageV,
  SolarPanelCurrentmA,
  OutsideHumidityPercent,
  AbsolutePressurehPa,
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
  int wifiRssiDbm = 0;

  bool mqttConnected = false;
  char mqttClientId[32] = {};

  SensorTelemetry latestSensor = {};
  uint32_t lastTelemetryUpdateMs = 0;
  uint32_t lastMqttReceiveMs = 0;
  uint32_t uptimeMs = 0;
};
