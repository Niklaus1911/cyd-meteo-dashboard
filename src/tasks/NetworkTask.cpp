#include "tasks/NetworkTask.h"

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>

#include "AppConfig.h"
#include "AppEvents.h"
#include "AppQueues.h"
#include "Log.h"
#include "app/AppStateStore.h"
#include "app/MqttSettings.h"

namespace {

EventGroupHandle_t s_systemEvents = nullptr;
QueueHandle_t s_commandQueue = nullptr;
QueueHandle_t s_mqttInboundQueue = nullptr;

WiFiClient s_wifiClient;
PubSubClient s_mqttClient(s_wifiClient);
MqttSettings s_mqttSettings;
MqttInboundMessage s_callbackMessage;
MqttInboundMessage s_processingMessage;

uint32_t s_lastWifiAttemptMs = 0;
uint32_t s_lastMqttAttemptMs = 0;
uint32_t s_lastIncompleteMqttLogMs = 0;
uint32_t s_lastDiagnosticLogMs = 0;
uint32_t s_lastStackLogMs = 0;
uint32_t s_lastRssiUpdateMs = 0;
uint32_t s_lastHeapUpdateMs = 0;
bool s_wifiWasConnected = false;
bool s_mqttWasConnected = false;
bool s_saveMqttSettingsRequested = false;
char s_mqttPortText[8] = {};
char s_lastWifiIpAddress[16] = {};

void logStackHighWaterMarkNow(const char* context);

void copyString(char* destination, size_t destinationSize, const char* source) {
  if (destinationSize == 0) {
    return;
  }

  const char* safeSource = source == nullptr ? "" : source;
  strncpy(destination, safeSource, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
}

void sanitizeZambrettiPayload(char* payload, size_t bufferSize) {
  if (payload == nullptr || bufferSize < 2) {
    return;
  }

  char temp[AppConfig::MqttPayloadMaxLength];
  size_t w = 0;
  size_t r = 0;

  while (payload[r] != '\0' && r < bufferSize && w < sizeof(temp) - 1) {
    const unsigned char c = static_cast<unsigned char>(payload[r]);

    if (c < 0x80) {
      if (c >= 0x20 || c == '\n') {
        temp[w++] = payload[r];
      }
      r++;
      continue;
    }

    if ((c & 0xE0) == 0xC0 && r + 1 < bufferSize && payload[r + 1] != '\0') {
      const unsigned char c2 = static_cast<unsigned char>(payload[r + 1]);
      if ((c2 & 0xC0) == 0x80) {
        if (c == 0xCE && c2 == 0x94) {
          if (w + 5 < sizeof(temp)) {
            temp[w++] = 'd';
            temp[w++] = 'e';
            temp[w++] = 'l';
            temp[w++] = 't';
            temp[w++] = 'a';
          }
          r += 2;
          continue;
        }
        if (c == 0xC2 || c == 0xC3) {
          temp[w++] = payload[r];
          temp[w++] = payload[r + 1];
          r += 2;
          continue;
        }
      }
      r += 2;
      continue;
    }

    if ((c & 0xF0) == 0xE0) {
      r += 3;
      continue;
    }

    if ((c & 0xF8) == 0xF0) {
      r += 4;
      continue;
    }

    r++;
  }

  temp[w] = '\0';
  strncpy(payload, temp, bufferSize - 1);
  payload[bufferSize - 1] = '\0';
}

const char* mqttStateReason(int state) {
  switch (state) {
    case MQTT_CONNECTION_TIMEOUT:
      return "connection timeout";
    case MQTT_CONNECTION_LOST:
      return "connection lost";
    case MQTT_CONNECT_FAILED:
      return "connect failed";
    case MQTT_DISCONNECTED:
      return "disconnected";
    case MQTT_CONNECTED:
      return "connected";
    case MQTT_CONNECT_BAD_PROTOCOL:
      return "bad protocol";
    case MQTT_CONNECT_BAD_CLIENT_ID:
      return "bad client id";
    case MQTT_CONNECT_UNAVAILABLE:
      return "broker unavailable";
    case MQTT_CONNECT_BAD_CREDENTIALS:
      return "bad credentials";
    case MQTT_CONNECT_UNAUTHORIZED:
      return "unauthorized";
    default:
      return "unknown";
  }
}

void logMqttSettings(const char* prefix) {
  LOG_TASK("%s MQTT settings complete=%d host='%s' port=%u user_set=%d",
           prefix,
           s_mqttSettings.isComplete(),
           s_mqttSettings.host,
           s_mqttSettings.port,
           s_mqttSettings.hasCredentials());
}

void publishMqttBrokerHost() {
  AppStateStore::setMqttBrokerHost(s_mqttSettings.host, 0);
}

void clearWifiDiagnostics() {
  copyString(s_lastWifiIpAddress, sizeof(s_lastWifiIpAddress), "");
  s_lastRssiUpdateMs = 0;
  AppStateStore::setWifiStatus(false, "", 0, 0);
  AppStateStore::setWifiIpAddress("", 0);
}

void updateWifiDiagnostics(uint32_t nowMs, bool force) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  const String ipAddress = WiFi.localIP().toString();
  const bool ipChanged = strcmp(s_lastWifiIpAddress, ipAddress.c_str()) != 0;
  const bool rssiDue = force || (nowMs - s_lastRssiUpdateMs >= AppConfig::RssiUpdatePeriodMs);

  if (rssiDue) {
    AppStateStore::setWifiStatus(true, WiFi.SSID().c_str(), WiFi.RSSI(), 0);
    s_lastRssiUpdateMs = nowMs;
  }

  if (force || ipChanged) {
    AppStateStore::setWifiIpAddress(ipAddress.c_str(), 0);
    copyString(s_lastWifiIpAddress, sizeof(s_lastWifiIpAddress), ipAddress.c_str());
  }
}

void updateHeapDiagnostic(uint32_t nowMs, bool force) {
  if (!force && nowMs - s_lastHeapUpdateMs < AppConfig::HeapUpdatePeriodMs) {
    return;
  }

  AppStateStore::updateFreeHeap(ESP.getFreeHeap(), 0);
  s_lastHeapUpdateMs = nowMs;
}

void setWifiEvent(bool connected) {
  if (s_systemEvents == nullptr) {
    return;
  }

  if (connected) {
    xEventGroupSetBits(s_systemEvents, AppEvents::WifiConnected);
  } else {
    xEventGroupClearBits(s_systemEvents, AppEvents::WifiConnected | AppEvents::MqttConnected);
  }
}

void setMqttEvent(bool connected) {
  if (s_systemEvents == nullptr) {
    return;
  }

  if (connected) {
    xEventGroupSetBits(s_systemEvents, AppEvents::MqttConnected);
  } else {
    xEventGroupClearBits(s_systemEvents, AppEvents::MqttConnected);
  }
}

void onWiFiManagerSaveConfig() {
  s_saveMqttSettingsRequested = true;
}

void onWiFiManagerConfigMode(WiFiManager* manager) {
  LOG_TASK("config portal active ssid='%s' ip=%s",
           manager->getConfigPortalSSID().c_str(),
           WiFi.softAPIP().toString().c_str());
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  if (s_mqttInboundQueue == nullptr) {
    return;
  }

  copyString(s_callbackMessage.topic, sizeof(s_callbackMessage.topic), topic);

  const uint32_t copyLength = min<uint32_t>(length, sizeof(s_callbackMessage.payload) - 1);
  memcpy(s_callbackMessage.payload, payload, copyLength);
  s_callbackMessage.payload[copyLength] = '\0';
  s_callbackMessage.payloadLength = copyLength;
  s_callbackMessage.receivedAtMs = millis();

  if (xQueueSend(s_mqttInboundQueue, &s_callbackMessage, 0) != pdTRUE) {
    LOG_TASK("MQTT inbound queue full; dropped topic='%s'", s_callbackMessage.topic);
  }
}

void configureMqttClientServer() {
  if (!s_mqttSettings.isComplete()) {
    return;
  }

  s_mqttClient.setServer(s_mqttSettings.host, s_mqttSettings.port);
}

void readPortalSettings(WiFiManagerParameter& host,
                        WiFiManagerParameter& port,
                        WiFiManagerParameter& username,
                        WiFiManagerParameter& password) {
  copyString(s_mqttSettings.host, sizeof(s_mqttSettings.host), host.getValue());
  copyString(s_mqttSettings.username, sizeof(s_mqttSettings.username), username.getValue());
  copyString(s_mqttSettings.password, sizeof(s_mqttSettings.password), password.getValue());

  const long parsedPort = strtol(port.getValue(), nullptr, 10);
  if (parsedPort > 0 && parsedPort <= UINT16_MAX) {
    s_mqttSettings.port = static_cast<uint16_t>(parsedPort);
  } else {
    s_mqttSettings.port = AppConfig::DefaultMqttPort;
  }
}

bool runWiFiManager(uint32_t nowMs) {
  s_lastWifiAttemptMs = nowMs;

  snprintf(s_mqttPortText, sizeof(s_mqttPortText), "%u", s_mqttSettings.port);

  WiFiManager* wifiManager = new WiFiManager();
  WiFiManagerParameter* mqttHost = new WiFiManagerParameter("mqtt_host",
                                                            "MQTT Broker",
                                                            s_mqttSettings.host,
                                                            sizeof(s_mqttSettings.host));
  WiFiManagerParameter* mqttPortParam = new WiFiManagerParameter("mqtt_port",
                                                                 "MQTT Port",
                                                                 s_mqttPortText,
                                                                 sizeof(s_mqttPortText));
  WiFiManagerParameter* mqttUser = new WiFiManagerParameter("mqtt_user",
                                                            "MQTT Username",
                                                            s_mqttSettings.username,
                                                            sizeof(s_mqttSettings.username));
  WiFiManagerParameter* mqttPass = new WiFiManagerParameter("mqtt_pass",
                                                            "MQTT Password",
                                                            s_mqttSettings.password,
                                                            sizeof(s_mqttSettings.password),
                                                            "type=\"password\"");

  if (wifiManager == nullptr || mqttHost == nullptr || mqttPortParam == nullptr ||
      mqttUser == nullptr || mqttPass == nullptr) {
    LOG_TASK("failed to allocate WiFiManager portal objects");
    delete wifiManager;
    delete mqttHost;
    delete mqttPortParam;
    delete mqttUser;
    delete mqttPass;
    return false;
  }

  s_saveMqttSettingsRequested = false;

  wifiManager->setDebugOutput(false);
  wifiManager->setConfigPortalTimeout(AppConfig::WifiManagerPortalTimeoutSec);
  wifiManager->setConnectTimeout(20);
  wifiManager->setSaveConfigCallback(onWiFiManagerSaveConfig);
  wifiManager->setAPCallback(onWiFiManagerConfigMode);
  wifiManager->addParameter(mqttHost);
  wifiManager->addParameter(mqttPortParam);
  wifiManager->addParameter(mqttUser);
  wifiManager->addParameter(mqttPass);

  LOG_TASK("starting WiFiManager auto-connect portal='%s'",
           AppConfig::WifiManagerPortalSsid);
  logStackHighWaterMarkNow("before WiFiManager portal");

  const bool connected = wifiManager->autoConnect(AppConfig::WifiManagerPortalSsid);

  LOG_TASK("WiFiManager portal ended connected=%d", connected);
  logStackHighWaterMarkNow("after WiFiManager portal");

  readPortalSettings(*mqttHost, *mqttPortParam, *mqttUser, *mqttPass);

  delete wifiManager;
  delete mqttHost;
  delete mqttPortParam;
  delete mqttUser;
  delete mqttPass;

  if (s_saveMqttSettingsRequested) {
    if (MqttSettingsStore::save(s_mqttSettings)) {
      LOG_TASK("saved MQTT settings to NVS");
      logMqttSettings("saved");
    } else {
      LOG_TASK("failed to save MQTT settings to NVS");
    }
  }

  configureMqttClientServer();
  publishMqttBrokerHost();

  if (!connected) {
    LOG_TASK("WiFiManager timed out or failed; will retry later");
    WiFi.disconnect(false, false);
    return false;
  }

  LOG_TASK("WiFiManager connected ip=%s", WiFi.localIP().toString().c_str());
  logMqttSettings("active");
  return true;
}

void maintainWifi(uint32_t nowMs) {
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    if (!s_wifiWasConnected) {
      const String ipAddress = WiFi.localIP().toString();
      LOG_TASK("WiFi connected ssid='%s' ip=%s rssi=%d",
               WiFi.SSID().c_str(),
               ipAddress.c_str(),
               WiFi.RSSI());
      updateWifiDiagnostics(nowMs, true);
    } else {
      updateWifiDiagnostics(nowMs, false);
    }

    setWifiEvent(true);
    s_wifiWasConnected = true;
    return;
  }

  if (s_wifiWasConnected) {
    LOG_TASK("WiFi disconnected");
    s_mqttClient.disconnect();
    clearWifiDiagnostics();
    AppStateStore::setMqttStatus(false, AppConfig::MqttClientId, 0);
  }

  setWifiEvent(false);
  s_wifiWasConnected = false;
  s_mqttWasConnected = false;

  if (nowMs - s_lastWifiAttemptMs >= AppConfig::WifiReconnectIntervalMs) {
    runWiFiManager(nowMs);
  }
}

void publishAvailability(const char* status) {
  if (!s_mqttClient.connected()) {
    return;
  }

  s_mqttClient.publish(AppConfig::MqttAvailabilityTopic, status, true);
}

bool mqttConnectWithWill() {
  const char* willTopic = AppConfig::MqttAvailabilityTopic;
  const char* willMessage = "offline";
  constexpr uint8_t willQos = 0;
  constexpr bool willRetain = true;

  if (s_mqttSettings.hasCredentials()) {
    return s_mqttClient.connect(AppConfig::MqttClientId,
                                s_mqttSettings.username,
                                s_mqttSettings.password,
                                willTopic,
                                willQos,
                                willRetain,
                                willMessage);
  }

  return s_mqttClient.connect(AppConfig::MqttClientId,
                              willTopic,
                              willQos,
                              willRetain,
                              willMessage);
}

struct EspHomeSensorTopic {
  const char* topic;
  SensorField field;
  const char* label;
};

constexpr EspHomeSensorTopic EspHomeSensorTopics[] = {
    {AppConfig::EspHomeBatteryTopic, SensorField::BatteryPercent, "battery"},
    {AppConfig::EspHomeOutsideTemperatureTopic, SensorField::OutsideTemperatureC, "outside_temperature"},
    {AppConfig::EspHomeSolarPanelVoltageTopic, SensorField::SolarPanelVoltageV, "solar_voltage"},
    {AppConfig::EspHomeSolarPanelCurrentTopic, SensorField::SolarPanelCurrentmA, "solar_current"},
    {AppConfig::EspHomeOutsideHumidityTopic, SensorField::OutsideHumidityPercent, "outside_humidity"},
    {AppConfig::EspHomeAbsolutePressureTopic, SensorField::AbsolutePressurehPa, "absolute_pressure"},
};

struct ForecastTopic {
  const char* topic;
  ForecastField field;
  const char* label;
};

constexpr ForecastTopic ForecastTopics[] = {
    {AppConfig::ForecastRegionTopic, ForecastField::Region, "region"},
    {AppConfig::ForecastAlertTopic, ForecastField::Alert, "alert"},
    {AppConfig::ForecastTextTopic, ForecastField::Text, "forecast"},
    {AppConfig::ForecastLowSummaryTopic, ForecastField::LowSummary, "low_summary"},
    {AppConfig::ForecastUpdatedTopic, ForecastField::Updated, "updated"},
};

const EspHomeSensorTopic* findEspHomeSensorTopic(const char* topic) {
  for (const EspHomeSensorTopic& sensorTopic : EspHomeSensorTopics) {
    if (strcmp(topic, sensorTopic.topic) == 0) {
      return &sensorTopic;
    }
  }

  return nullptr;
}

const ForecastTopic* findForecastTopic(const char* topic) {
  for (const ForecastTopic& forecastTopic : ForecastTopics) {
    if (strcmp(topic, forecastTopic.topic) == 0) {
      return &forecastTopic;
    }
  }

  return nullptr;
}

bool subscribeEspHomeTopics() {
  bool allSubscribed = true;

  for (const EspHomeSensorTopic& sensorTopic : EspHomeSensorTopics) {
    if (s_mqttClient.subscribe(sensorTopic.topic)) {
      LOG_TASK("subscribed ESPHome topic='%s'", sensorTopic.topic);
    } else {
      LOG_TASK("failed to subscribe ESPHome topic='%s'", sensorTopic.topic);
      allSubscribed = false;
    }
  }

  return allSubscribed;
}

bool subscribeForecastTopics() {
  bool allSubscribed = true;

  for (const ForecastTopic& forecastTopic : ForecastTopics) {
    if (s_mqttClient.subscribe(forecastTopic.topic)) {
      LOG_TASK("subscribed forecast topic='%s'", forecastTopic.topic);
    } else {
      LOG_TASK("failed to subscribe forecast topic='%s'", forecastTopic.topic);
      allSubscribed = false;
    }
  }

  return allSubscribed;
}

void resetCredentialsAndRestart() {
  LOG_TASK("credential reset requested; clearing WiFiManager WiFi and MQTT settings");
  AppStateStore::setCredentialResetStatus(true, true, false, 0);

  if (s_mqttClient.connected()) {
    publishAvailability("offline");
    s_mqttClient.disconnect();
  }

  WiFi.disconnect(true, true);
  clearWifiDiagnostics();
  AppStateStore::setMqttStatus(false, AppConfig::MqttClientId, 0);
  setWifiEvent(false);
  setMqttEvent(false);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.resetSettings();

  if (MqttSettingsStore::clear()) {
    LOG_TASK("MQTT settings cleared from NVS for credential reset");
  } else {
    LOG_TASK("failed to clear MQTT settings from NVS for credential reset");
  }

  s_mqttSettings = {};
  publishMqttBrokerHost();
  AppStateStore::setCredentialResetStatus(true, true, true, 0);

  LOG_TASK("restarting after credential reset; setup AP='%s'",
           AppConfig::WifiManagerPortalSsid);
  vTaskDelay(pdMS_TO_TICKS(AppConfig::RestartAfterConfigResetMs));
  ESP.restart();
}

void maintainMqtt(uint32_t nowMs) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (s_mqttClient.connected()) {
    if (!s_mqttWasConnected) {
      LOG_TASK("MQTT connected");
      AppStateStore::setMqttStatus(true, AppConfig::MqttClientId, 0);
    }

    setMqttEvent(true);
    s_mqttWasConnected = true;
    s_mqttClient.loop();
    return;
  }

  if (s_mqttWasConnected) {
    const int state = s_mqttClient.state();
    LOG_TASK("MQTT disconnected state=%d reason='%s'", state, mqttStateReason(state));
    AppStateStore::setMqttStatus(false, AppConfig::MqttClientId, 0);
  }

  setMqttEvent(false);
  s_mqttWasConnected = false;

  if (!s_mqttSettings.isComplete()) {
    if (nowMs - s_lastIncompleteMqttLogMs >= AppConfig::StatusLogPeriodMs) {
      s_lastIncompleteMqttLogMs = nowMs;
      LOG_TASK("MQTT config incomplete; connect WiFi only until portal settings are saved");
    }
    return;
  }

  if (nowMs - s_lastMqttAttemptMs < AppConfig::MqttReconnectIntervalMs) {
    return;
  }

  s_lastMqttAttemptMs = nowMs;
  configureMqttClientServer();
  LOG_TASK("connecting MQTT broker=%s:%u for hardcoded ESPHome topics",
           s_mqttSettings.host,
           s_mqttSettings.port);

  if (!mqttConnectWithWill()) {
    const int state = s_mqttClient.state();
    LOG_TASK("MQTT connect failed state=%d reason='%s'", state, mqttStateReason(state));
    return;
  }

  AppStateStore::setMqttStatus(true, AppConfig::MqttClientId, 0);
  setMqttEvent(true);
  s_mqttWasConnected = true;

  publishAvailability("online");
  LOG_TASK("MQTT connected broker=%s:%u reason='%s'",
           s_mqttSettings.host,
           s_mqttSettings.port,
           mqttStateReason(s_mqttClient.state()));

  subscribeEspHomeTopics();
  subscribeForecastTopics();
}

void parseTelemetryMessage(const MqttInboundMessage& message) {
  const ForecastTopic* forecastTopic = findForecastTopic(message.topic);
  if (forecastTopic != nullptr) {
    char sanitized[AppConfig::MqttPayloadMaxLength];
    copyString(sanitized, sizeof(sanitized), message.payload);
    sanitizeZambrettiPayload(sanitized, sizeof(sanitized));
    if (!AppStateStore::updateForecastValue(forecastTopic->field,
                                            sanitized,
                                            message.receivedAtMs,
                                            0)) {
      LOG_TASK("failed to update AppState for forecast label='%s'", forecastTopic->label);
      return;
    }

    if (s_systemEvents != nullptr) {
      xEventGroupSetBits(s_systemEvents, AppEvents::AppStateUpdated);
    }

    LOG_TASK("forecast updated label='%s' received_at_ms=%lu payload_len=%lu",
             forecastTopic->label,
             static_cast<unsigned long>(message.receivedAtMs),
             static_cast<unsigned long>(message.payloadLength));
    return;
  }

  const EspHomeSensorTopic* sensorTopic = findEspHomeSensorTopic(message.topic);
  if (sensorTopic == nullptr) {
    LOG_TASK("ignored unknown MQTT topic='%s'", message.topic);
    return;
  }

  errno = 0;
  char* end = nullptr;
  const float value = strtof(message.payload, &end);

  if (end == message.payload || errno == ERANGE) {
    LOG_TASK("invalid numeric payload topic='%s' payload='%s'", message.topic, message.payload);
    return;
  }

  while (*end != '\0') {
    if (!isspace(static_cast<unsigned char>(*end))) {
      LOG_TASK("invalid numeric payload topic='%s' payload='%s'", message.topic, message.payload);
      return;
    }
    ++end;
  }

  if (!AppStateStore::updateSensorValue(sensorTopic->field, value, message.receivedAtMs, 0)) {
    LOG_TASK("failed to update AppState for ESPHome sensor label='%s'", sensorTopic->label);
    return;
  }

  if (s_systemEvents != nullptr) {
    xEventGroupSetBits(s_systemEvents, AppEvents::AppStateUpdated);
  }

  LOG_TASK("ESPHome sensor updated label='%s' value=%.3f received_at_ms=%lu telemetry_stale=0",
           sensorTopic->label,
           value,
           static_cast<unsigned long>(message.receivedAtMs));
}

void processInboundMqtt() {
  if (s_mqttInboundQueue == nullptr) {
    return;
  }

  while (xQueueReceive(s_mqttInboundQueue, &s_processingMessage, 0) == pdTRUE) {
    parseTelemetryMessage(s_processingMessage);
  }
}

void updateTelemetryStaleState(uint32_t nowMs) {
  AppState snapshot;
  if (!AppStateStore::copy(snapshot, 0)) {
    return;
  }

  if (!snapshot.latestSensor.valid || snapshot.latestSensor.stale ||
      snapshot.lastMqttReceiveMs == 0) {
    return;
  }

  const uint32_t telemetryAgeMs = static_cast<uint32_t>(nowMs - snapshot.lastMqttReceiveMs);
  if (telemetryAgeMs >= AppConfig::TelemetryStaleAfterMs) {
    AppStateStore::setTelemetryStale(true, 0);
    LOG_TASK("telemetry marked stale age_ms=%lu last_receive_ms=%lu valid=%d stale=1",
             static_cast<unsigned long>(telemetryAgeMs),
             static_cast<unsigned long>(snapshot.lastMqttReceiveMs),
             snapshot.latestSensor.valid);
  }
}

void logRuntimeDiagnostics(uint32_t nowMs) {
  if (nowMs - s_lastDiagnosticLogMs < AppConfig::DiagnosticLogPeriodMs) {
    return;
  }

  s_lastDiagnosticLogMs = nowMs;

  AppState snapshot;
  const bool copied = AppStateStore::copy(snapshot, 0);
  const bool telemetryValid = copied ? snapshot.latestSensor.valid : false;
  const bool telemetryStale = copied ? snapshot.latestSensor.stale : true;
  const uint32_t lastReceiveMs = copied ? snapshot.lastMqttReceiveMs : 0;
  const uint32_t uptimeMs = copied ? snapshot.uptimeMs : nowMs;

  LOG_TASK("diag heap=%lu wifi=%d mqtt=%d telemetry_valid=%d telemetry_stale=%d last_receive_ms=%lu uptime_ms=%lu",
           static_cast<unsigned long>(ESP.getFreeHeap()),
           WiFi.status() == WL_CONNECTED,
           s_mqttClient.connected(),
           telemetryValid,
           telemetryStale,
           static_cast<unsigned long>(lastReceiveMs),
           static_cast<unsigned long>(uptimeMs));
}

void logStackHighWaterMark(uint32_t nowMs) {
  if (nowMs - s_lastStackLogMs < AppConfig::StackLogPeriodMs) {
    return;
  }

  s_lastStackLogMs = nowMs;

  const UBaseType_t highWaterWords = uxTaskGetStackHighWaterMark(nullptr);
  const uint32_t highWaterBytes = highWaterWords * sizeof(StackType_t);
  LOG_TASK("stack high-water free=%lu bytes (%lu words)",
           static_cast<unsigned long>(highWaterBytes),
           static_cast<unsigned long>(highWaterWords));
}

void logStackHighWaterMarkNow(const char* context) {
  const UBaseType_t highWaterWords = uxTaskGetStackHighWaterMark(nullptr);
  const uint32_t highWaterBytes = highWaterWords * sizeof(StackType_t);
  LOG_TASK("stack high-water %s free=%lu bytes (%lu words)",
           context,
           static_cast<unsigned long>(highWaterBytes),
           static_cast<unsigned long>(highWaterWords));
}

void drainCommandQueue() {
  if (s_commandQueue == nullptr) {
    return;
  }

  CommandMessage command;
  while (xQueueReceive(s_commandQueue, &command, 0) == pdTRUE) {
    switch (command.type) {
      case CommandType::ResetCredentials:
        resetCredentialsAndRestart();
        break;
      case CommandType::PublishMqttCommand:
      default:
        LOG_TASK("queued command topic='%s' payload='%s'", command.topic, command.payload);
        break;
    }
  }
}

void networkTaskMain(void*) {
  LOG_TASK("started");
  logStackHighWaterMarkNow("at start");

  if (MqttSettingsStore::load(s_mqttSettings)) {
    LOG_TASK("loaded MQTT settings from NVS");
    logMqttSettings("loaded");
  } else {
    LOG_TASK("MQTT settings unavailable; using defaults");
    logMqttSettings("default");
  }
  publishMqttBrokerHost();

  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);

  s_mqttClient.setCallback(mqttCallback);
  s_mqttClient.setBufferSize(AppConfig::MqttPayloadMaxLength);
  s_mqttClient.setKeepAlive(AppConfig::MqttKeepAliveSec);
  s_mqttClient.setSocketTimeout(AppConfig::MqttSocketTimeoutSec);
  configureMqttClientServer();

  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastStatusLogMs = 0;

  clearWifiDiagnostics();
  AppStateStore::setMqttStatus(false, AppConfig::MqttClientId);
  updateHeapDiagnostic(millis(), true);

  runWiFiManager(millis());

  for (;;) {
    const uint32_t nowMs = millis();

    AppStateStore::updateUptime(nowMs, 0);
    updateHeapDiagnostic(nowMs, false);
    maintainWifi(nowMs);
    maintainMqtt(nowMs);
    processInboundMqtt();
    updateTelemetryStaleState(millis());
    drainCommandQueue();
    logRuntimeDiagnostics(nowMs);
    logStackHighWaterMark(nowMs);

    if (nowMs - lastStatusLogMs >= AppConfig::StatusLogPeriodMs) {
      lastStatusLogMs = nowMs;
      LOG_TASK("network active wifi=%d mqtt=%d", WiFi.status() == WL_CONNECTED, s_mqttClient.connected());
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::NetworkLoopPeriodMs));
  }
}

}  // namespace

bool startNetworkTask(EventGroupHandle_t systemEvents,
                      QueueHandle_t commandQueue,
                      QueueHandle_t mqttInboundQueue) {
  s_systemEvents = systemEvents;
  s_commandQueue = commandQueue;
  s_mqttInboundQueue = mqttInboundQueue;

  const BaseType_t result = xTaskCreatePinnedToCore(
      networkTaskMain,
      AppConfig::NetworkTaskName,
      AppConfig::NetworkTaskStackBytes,
      nullptr,
      AppConfig::NetworkTaskPriority,
      nullptr,
      AppConfig::NetworkTaskCore);

  return result == pdPASS;
}
