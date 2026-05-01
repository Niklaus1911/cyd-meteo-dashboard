#include "tasks/UiTask.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "AppQueues.h"
#include "Log.h"
#include "app/AppStateStore.h"
#include "ui/DashboardScreen.h"
#include "ui/LvglPort.h"

namespace {

EventGroupHandle_t s_systemEvents = nullptr;
QueueHandle_t s_commandQueue = nullptr;

void uiTaskMain(void*) {
  LOG_TASK("started");
  LvglPort::begin();
  DashboardScreen::create();

  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastStatusLogMs = 0;
  uint32_t lastUiUpdateMs = 0;

  for (;;) {
    const uint32_t nowMs = millis();

    AppState snapshot;
    const bool hasSnapshot = AppStateStore::copy(snapshot, 0);

    if (hasSnapshot && nowMs - lastUiUpdateMs >= AppConfig::UiRefreshPeriodMs) {
      lastUiUpdateMs = nowMs;
      DashboardScreen::update(snapshot);
    }

    if (hasSnapshot && nowMs - lastStatusLogMs >= AppConfig::StatusLogPeriodMs) {
      lastStatusLogMs = nowMs;
      LOG_TASK("LVGL UI active; wifi=%d mqtt=%d telemetry_valid=%d telemetry_stale=%d uptime=%lu",
               snapshot.wifiConnected,
               snapshot.mqttConnected,
               snapshot.latestSensor.valid,
               snapshot.latestSensor.stale,
               static_cast<unsigned long>(snapshot.uptimeMs));
    }

    LvglPort::tick();
    LvglPort::handleTimers();

    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::UiLoopPeriodMs));
  }
}

}  // namespace

bool startUiTask(EventGroupHandle_t systemEvents, QueueHandle_t commandQueue) {
  s_systemEvents = systemEvents;
  s_commandQueue = commandQueue;
  (void)s_systemEvents;
  (void)s_commandQueue;

  const BaseType_t result = xTaskCreatePinnedToCore(
      uiTaskMain,
      AppConfig::UiTaskName,
      AppConfig::UiTaskStackWords,
      nullptr,
      AppConfig::UiTaskPriority,
      nullptr,
      AppConfig::UiTaskCore);

  return result == pdPASS;
}
