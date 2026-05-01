#pragma once

#include <Arduino.h>

struct SensorTelemetry {
  bool valid = false;
  bool stale = true;
  char source[32] = {};
  float temperatureC = NAN;
  float humidityPct = NAN;
  float pressureHpa = NAN;
  float batteryPct = NAN;
  int rssiDbm = 0;
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
