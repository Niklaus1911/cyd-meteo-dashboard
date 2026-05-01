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

uint32_t s_lastStackLogMs = 0;

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

void uiTaskMain(void*) {
  LOG_TASK("started");
  logStackHighWaterMarkNow("at start");
  LvglPort::begin();
  DashboardScreen::create();
  logStackHighWaterMarkNow("after LVGL init");

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
    logStackHighWaterMark(nowMs);

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
      AppConfig::UiTaskStackBytes,
      nullptr,
      AppConfig::UiTaskPriority,
      nullptr,
      AppConfig::UiTaskCore);

  return result == pdPASS;
}
