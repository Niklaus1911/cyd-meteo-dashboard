#include "app/AppStateStore.h"

#include <cstring>

namespace {

AppState s_state;
SemaphoreHandle_t s_mutex = nullptr;

void copyString(char* destination, size_t destinationSize, const char* source) {
  if (destinationSize == 0) {
    return;
  }

  const char* safeSource = source == nullptr ? "" : source;
  strncpy(destination, safeSource, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
}

}  // namespace

namespace AppStateStore {

bool begin() {
  if (s_mutex != nullptr) {
    return true;
  }

  s_mutex = xSemaphoreCreateMutex();
  return s_mutex != nullptr;
}

bool copy(AppState& out, TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  out = s_state;
  xSemaphoreGive(s_mutex);
  return true;
}

bool setWifiStatus(bool connected,
                   const char* ssid,
                   int rssiDbm,
                   TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  s_state.wifiConnected = connected;
  copyString(s_state.wifiSsid, sizeof(s_state.wifiSsid), connected ? ssid : "");
  s_state.wifiRssiDbm = connected ? rssiDbm : 0;

  xSemaphoreGive(s_mutex);
  return true;
}

bool setMqttStatus(bool connected, const char* clientId, TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  s_state.mqttConnected = connected;
  copyString(s_state.mqttClientId, sizeof(s_state.mqttClientId), connected ? clientId : "");

  xSemaphoreGive(s_mutex);
  return true;
}

bool setMqttBrokerHost(const char* host, TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  copyString(s_state.mqttBrokerHost, sizeof(s_state.mqttBrokerHost), host);

  xSemaphoreGive(s_mutex);
  return true;
}

bool setCredentialResetStatus(bool requested,
                              bool resetting,
                              bool rebooting,
                              TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  s_state.credentialResetRequested = requested;
  s_state.credentialResetting = resetting;
  s_state.credentialRebooting = rebooting;

  xSemaphoreGive(s_mutex);
  return true;
}

bool updateTelemetry(const SensorTelemetry& telemetry,
                     uint32_t updateMs,
                     TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  s_state.latestSensor = telemetry;
  s_state.latestSensor.valid = true;
  s_state.latestSensor.stale = false;
  s_state.lastTelemetryUpdateMs = updateMs;

  xSemaphoreGive(s_mutex);
  return true;
}

bool updateSensorValue(SensorField field,
                       float value,
                       uint32_t updateMs,
                       TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  switch (field) {
    case SensorField::BatteryPercent:
      s_state.latestSensor.batteryPercent = value;
      s_state.latestSensor.batteryPercentValid = true;
      break;
    case SensorField::OutsideTemperatureC:
      s_state.latestSensor.outsideTemperatureC = value;
      s_state.latestSensor.outsideTemperatureValid = true;
      break;
    case SensorField::SolarPanelVoltageV:
      s_state.latestSensor.solarPanelVoltageV = value;
      s_state.latestSensor.solarPanelVoltageValid = true;
      break;
    case SensorField::SolarPanelCurrentmA:
      s_state.latestSensor.solarPanelCurrentmA = value;
      s_state.latestSensor.solarPanelCurrentValid = true;
      break;
    case SensorField::OutsideHumidityPercent:
      s_state.latestSensor.outsideHumidityPercent = value;
      s_state.latestSensor.outsideHumidityValid = true;
      break;
    case SensorField::AbsolutePressurehPa:
      s_state.latestSensor.absolutePressurehPa = value;
      s_state.latestSensor.absolutePressureValid = true;
      break;
  }

  s_state.latestSensor.valid = true;
  s_state.latestSensor.stale = false;
  s_state.lastTelemetryUpdateMs = updateMs;
  s_state.lastMqttReceiveMs = updateMs;

  xSemaphoreGive(s_mutex);
  return true;
}

bool setTelemetryStale(bool stale, TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  s_state.latestSensor.stale = stale;

  xSemaphoreGive(s_mutex);
  return true;
}

bool recordMqttReceive(uint32_t receiveMs, TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  s_state.lastMqttReceiveMs = receiveMs;
  if (s_state.latestSensor.valid) {
    s_state.latestSensor.stale = false;
  }

  xSemaphoreGive(s_mutex);
  return true;
}

bool updateUptime(uint32_t uptimeMs, TickType_t timeoutTicks) {
  if (s_mutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(s_mutex, timeoutTicks) != pdTRUE) {
    return false;
  }

  s_state.uptimeMs = uptimeMs;

  xSemaphoreGive(s_mutex);
  return true;
}

SemaphoreHandle_t mutexHandle() {
  return s_mutex;
}

}  // namespace AppStateStore
