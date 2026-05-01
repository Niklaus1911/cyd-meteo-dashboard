#include "tasks/UiTask.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "AppQueues.h"
#include "Log.h"
#include "app/AppStateStore.h"
#include "display/DisplayDiagnostic.h"

namespace {

EventGroupHandle_t s_systemEvents = nullptr;
QueueHandle_t s_commandQueue = nullptr;

void uiTaskMain(void*) {
  LOG_TASK("started");
  DisplayDiagnostic::begin();

  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastStatusLogMs = 0;
  uint32_t lastDisplayConfigLogMs = 0;

  for (;;) {
    const uint32_t nowMs = millis();

    AppState snapshot;
    if (AppStateStore::copy(snapshot, 0) && nowMs - lastStatusLogMs >= AppConfig::StatusLogPeriodMs) {
      lastStatusLogMs = nowMs;
      LOG_TASK("UI skeleton active; wifi=%d mqtt=%d telemetry=%d uptime=%lu",
               snapshot.wifiConnected,
               snapshot.mqttConnected,
               snapshot.latestSensor.valid,
               static_cast<unsigned long>(snapshot.uptimeMs));
    }

    if (nowMs - lastDisplayConfigLogMs >= AppConfig::DiagnosticLogPeriodMs) {
      lastDisplayConfigLogMs = nowMs;
      DisplayDiagnostic::logConfig();
    }

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
