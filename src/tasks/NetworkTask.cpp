#include "tasks/NetworkTask.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "AppEvents.h"
#include "AppQueues.h"
#include "Log.h"
#include "app/AppStateStore.h"

namespace {

EventGroupHandle_t s_systemEvents = nullptr;
QueueHandle_t s_commandQueue = nullptr;

void syncConnectionStateFromEvents() {
  if (s_systemEvents == nullptr) {
    return;
  }

  const EventBits_t bits = xEventGroupGetBits(s_systemEvents);
  const bool wifiConnected = (bits & AppEvents::WifiConnected) != 0;
  const bool mqttConnected = (bits & AppEvents::MqttConnected) != 0;

  AppStateStore::setWifiStatus(wifiConnected, "", 0, 0);
  AppStateStore::setMqttStatus(mqttConnected, AppConfig::MqttClientId, 0);
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

  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastStatusLogMs = 0;

  AppStateStore::setWifiStatus(false, "", 0);
  AppStateStore::setMqttStatus(false, AppConfig::MqttClientId);

  for (;;) {
    const uint32_t nowMs = millis();

    AppStateStore::updateUptime(nowMs, 0);
    syncConnectionStateFromEvents();
    drainCommandQueue();

    if (nowMs - lastStatusLogMs >= AppConfig::StatusLogPeriodMs) {
      lastStatusLogMs = nowMs;
      LOG_TASK("network skeleton active; WiFi/MQTT implementation pending");
    }

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::NetworkLoopPeriodMs));
  }
}

}  // namespace

bool startNetworkTask(EventGroupHandle_t systemEvents, QueueHandle_t commandQueue) {
  s_systemEvents = systemEvents;
  s_commandQueue = commandQueue;

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
