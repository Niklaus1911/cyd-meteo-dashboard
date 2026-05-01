#include "tasks/NetworkTask.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiManager.h>

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

uint32_t s_lastWifiAttemptMs = 0;
uint32_t s_lastMqttAttemptMs = 0;
uint32_t s_lastIncompleteMqttLogMs = 0;
bool s_wifiWasConnected = false;
bool s_mqttWasConnected = false;
bool s_saveMqttSettingsRequested = false;

void copyString(char* destination, size_t destinationSize, const char* source) {
  if (destinationSize == 0) {
    return;
  }

  const char* safeSource = source == nullptr ? "" : source;
  strncpy(destination, safeSource, destinationSize - 1);
  destination[destinationSize - 1] = '\0';
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

  MqttInboundMessage message;
  copyString(message.topic, sizeof(message.topic), topic);

  const uint32_t copyLength = min<uint32_t>(length, sizeof(message.payload) - 1);
  memcpy(message.payload, payload, copyLength);
  message.payload[copyLength] = '\0';
  message.payloadLength = copyLength;
  message.receivedAtMs = millis();

  if (xQueueSend(s_mqttInboundQueue, &message, 0) != pdTRUE) {
    LOG_TASK("MQTT inbound queue full; dropped topic='%s'", message.topic);
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
                        WiFiManagerParameter& password,
                        WiFiManagerParameter& topic) {
  copyString(s_mqttSettings.host, sizeof(s_mqttSettings.host), host.getValue());
  copyString(s_mqttSettings.username, sizeof(s_mqttSettings.username), username.getValue());
  copyString(s_mqttSettings.password, sizeof(s_mqttSettings.password), password.getValue());
  copyString(s_mqttSettings.telemetryTopic, sizeof(s_mqttSettings.telemetryTopic), topic.getValue());

  const long parsedPort = strtol(port.getValue(), nullptr, 10);
  if (parsedPort > 0 && parsedPort <= UINT16_MAX) {
    s_mqttSettings.port = static_cast<uint16_t>(parsedPort);
  } else {
    s_mqttSettings.port = AppConfig::DefaultMqttPort;
  }
}

bool runWiFiManager(uint32_t nowMs) {
  s_lastWifiAttemptMs = nowMs;

  char mqttPort[8] = {};
  snprintf(mqttPort, sizeof(mqttPort), "%u", s_mqttSettings.port);

  WiFiManagerParameter mqttHost("mqtt_host",
                                "MQTT Broker",
                                s_mqttSettings.host,
                                sizeof(s_mqttSettings.host));
  WiFiManagerParameter mqttPortParam("mqtt_port",
                                     "MQTT Port",
                                     mqttPort,
                                     sizeof(mqttPort));
  WiFiManagerParameter mqttUser("mqtt_user",
                                "MQTT Username",
                                s_mqttSettings.username,
                                sizeof(s_mqttSettings.username));
  WiFiManagerParameter mqttPass("mqtt_pass",
                                "MQTT Password",
                                s_mqttSettings.password,
                                sizeof(s_mqttSettings.password),
                                "type=\"password\"");
  WiFiManagerParameter mqttTopic("mqtt_topic",
                                 "Telemetry Topic",
                                 s_mqttSettings.telemetryTopic,
                                 sizeof(s_mqttSettings.telemetryTopic));

  s_saveMqttSettingsRequested = false;

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.setConfigPortalTimeout(AppConfig::WifiManagerPortalTimeoutSec);
  wifiManager.setConnectTimeout(20);
  wifiManager.setSaveConfigCallback(onWiFiManagerSaveConfig);
  wifiManager.setAPCallback(onWiFiManagerConfigMode);
  wifiManager.addParameter(&mqttHost);
  wifiManager.addParameter(&mqttPortParam);
  wifiManager.addParameter(&mqttUser);
  wifiManager.addParameter(&mqttPass);
  wifiManager.addParameter(&mqttTopic);

  LOG_TASK("starting WiFiManager auto-connect portal='%s'",
           AppConfig::WifiManagerPortalSsid);

  const bool connected = wifiManager.autoConnect(AppConfig::WifiManagerPortalSsid);

  readPortalSettings(mqttHost, mqttPortParam, mqttUser, mqttPass, mqttTopic);

  if (s_saveMqttSettingsRequested) {
    if (MqttSettingsStore::save(s_mqttSettings)) {
      LOG_TASK("saved MQTT settings to NVS");
    } else {
      LOG_TASK("failed to save MQTT settings to NVS");
    }
  }

  configureMqttClientServer();

  if (!connected) {
    LOG_TASK("WiFiManager timed out or failed; will retry later");
    WiFi.disconnect(false, false);
    return false;
  }

  LOG_TASK("WiFiManager connected ip=%s", WiFi.localIP().toString().c_str());
  return true;
}

void maintainWifi(uint32_t nowMs) {
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    if (!s_wifiWasConnected) {
      LOG_TASK("WiFi connected ssid='%s' ip=%s rssi=%d",
               WiFi.SSID().c_str(),
               WiFi.localIP().toString().c_str(),
               WiFi.RSSI());
    }

    AppStateStore::setWifiStatus(true, WiFi.SSID().c_str(), WiFi.RSSI(), 0);
    setWifiEvent(true);
    s_wifiWasConnected = true;
    return;
  }

  if (s_wifiWasConnected) {
    LOG_TASK("WiFi disconnected");
    s_mqttClient.disconnect();
  }

  AppStateStore::setWifiStatus(false, "", 0, 0);
  AppStateStore::setMqttStatus(false, AppConfig::MqttClientId, 0);
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

void maintainMqtt(uint32_t nowMs) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (s_mqttClient.connected()) {
    if (!s_mqttWasConnected) {
      LOG_TASK("MQTT connected");
    }

    AppStateStore::setMqttStatus(true, AppConfig::MqttClientId, 0);
    setMqttEvent(true);
    s_mqttWasConnected = true;
    s_mqttClient.loop();
    return;
  }

  if (s_mqttWasConnected) {
    LOG_TASK("MQTT disconnected state=%d", s_mqttClient.state());
  }

  AppStateStore::setMqttStatus(false, AppConfig::MqttClientId, 0);
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
  LOG_TASK("connecting MQTT broker=%s:%u", s_mqttSettings.host, s_mqttSettings.port);

  if (!mqttConnectWithWill()) {
    LOG_TASK("MQTT connect failed state=%d", s_mqttClient.state());
    return;
  }

  AppStateStore::setMqttStatus(true, AppConfig::MqttClientId, 0);
  setMqttEvent(true);
  s_mqttWasConnected = true;

  publishAvailability("online");

  if (s_mqttClient.subscribe(s_mqttSettings.telemetryTopic)) {
    LOG_TASK("subscribed telemetry topic='%s'", s_mqttSettings.telemetryTopic);
  } else {
    LOG_TASK("failed to subscribe telemetry topic='%s'", s_mqttSettings.telemetryTopic);
  }
}

bool readFloat(JsonObjectConst root, const char* key, float& target) {
  JsonVariantConst value = root[key];
  if (value.isNull() || !value.is<float>()) {
    return false;
  }

  target = value.as<float>();
  return true;
}

bool readInt(JsonObjectConst root, const char* key, int& target) {
  JsonVariantConst value = root[key];
  if (value.isNull() || !value.is<int>()) {
    return false;
  }

  target = value.as<int>();
  return true;
}

void parseTelemetryMessage(const MqttInboundMessage& message) {
  AppStateStore::recordMqttReceive(message.receivedAtMs, 0);

  JsonDocument document;
  const DeserializationError error =
      deserializeJson(document, message.payload, message.payloadLength);

  if (error) {
    LOG_TASK("invalid MQTT JSON topic='%s' error=%s", message.topic, error.c_str());
    return;
  }

  JsonObjectConst root = document.as<JsonObjectConst>();
  if (root.isNull()) {
    LOG_TASK("MQTT payload is not a JSON object topic='%s'", message.topic);
    return;
  }

  SensorTelemetry telemetry;
  telemetry.valid = true;
  telemetry.stale = false;
  copyString(telemetry.source, sizeof(telemetry.source), message.topic);

  JsonVariantConst source = root["source"];
  if (!source.isNull() && source.is<const char*>()) {
    copyString(telemetry.source, sizeof(telemetry.source), source.as<const char*>());
  }

  readFloat(root, "temperature", telemetry.temperatureC);
  readFloat(root, "temperatureC", telemetry.temperatureC);
  readFloat(root, "temp", telemetry.temperatureC);
  readFloat(root, "humidity", telemetry.humidityPct);
  readFloat(root, "humidityPct", telemetry.humidityPct);
  readFloat(root, "pressure", telemetry.pressureHpa);
  readFloat(root, "pressureHpa", telemetry.pressureHpa);
  readFloat(root, "battery", telemetry.batteryPct);
  readFloat(root, "batteryPct", telemetry.batteryPct);
  readInt(root, "rssi", telemetry.rssiDbm);
  readInt(root, "rssiDbm", telemetry.rssiDbm);

  AppStateStore::updateTelemetry(telemetry, message.receivedAtMs, 0);

  if (s_systemEvents != nullptr) {
    xEventGroupSetBits(s_systemEvents, AppEvents::AppStateUpdated);
  }

  LOG_TASK("telemetry updated source='%s'", telemetry.source);
}

void processInboundMqtt() {
  if (s_mqttInboundQueue == nullptr) {
    return;
  }

  MqttInboundMessage message;
  while (xQueueReceive(s_mqttInboundQueue, &message, 0) == pdTRUE) {
    parseTelemetryMessage(message);
  }
}

void updateTelemetryStaleState(uint32_t nowMs) {
  AppState snapshot;
  if (!AppStateStore::copy(snapshot, 0)) {
    return;
  }

  if (!snapshot.latestSensor.valid || snapshot.latestSensor.stale) {
    return;
  }

  if (nowMs - snapshot.lastTelemetryUpdateMs >= AppConfig::TelemetryStaleAfterMs) {
    AppStateStore::setTelemetryStale(true, 0);
    LOG_TASK("telemetry marked stale after %lu ms",
             static_cast<unsigned long>(AppConfig::TelemetryStaleAfterMs));
  }
}

void drainCommandQueue() {
  if (s_commandQueue == nullptr) {
    return;
  }

  CommandMessage command;
  while (xQueueReceive(s_commandQueue, &command, 0) == pdTRUE) {
    LOG_TASK("queued command topic='%s' payload='%s'", command.topic, command.payload);
  }
}

void networkTaskMain(void*) {
  LOG_TASK("started");

  if (MqttSettingsStore::load(s_mqttSettings)) {
    LOG_TASK("loaded MQTT settings from NVS complete=%d", s_mqttSettings.isComplete());
  } else {
    LOG_TASK("failed to load MQTT settings from NVS; using defaults");
  }

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

  AppStateStore::setWifiStatus(false, "", 0);
  AppStateStore::setMqttStatus(false, AppConfig::MqttClientId);

  runWiFiManager(millis());

  for (;;) {
    const uint32_t nowMs = millis();

    AppStateStore::updateUptime(nowMs, 0);
    maintainWifi(nowMs);
    maintainMqtt(nowMs);
    processInboundMqtt();
    updateTelemetryStaleState(nowMs);
    drainCommandQueue();

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
      AppConfig::NetworkTaskStackWords,
      nullptr,
      AppConfig::NetworkTaskPriority,
      nullptr,
      AppConfig::NetworkTaskCore);

  return result == pdPASS;
}
