#include <Arduino.h>

#include "AppConfig.h"
#include "AppQueues.h"
#include "AppRuntime.h"
#include "Log.h"
#include "app/AppStateStore.h"
#include "tasks/NetworkTask.h"
#include "tasks/UiTask.h"

namespace {

void haltBoot(const char* reason) {
  Serial.printf("[boot] fatal: %s\n", reason);
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

}  // namespace

void setup() {
  Serial.begin(AppConfig::SerialBaud);
  vTaskDelay(pdMS_TO_TICKS(200));

  Serial.printf("\n[boot] CYD dashboard firmware skeleton starting on core %d\n", xPortGetCoreID());

  if (!AppStateStore::begin()) {
    haltBoot("failed to create AppState mutex");
  }

  g_systemEvents = xEventGroupCreate();
  if (g_systemEvents == nullptr) {
    haltBoot("failed to create system event group");
  }

  g_commandQueue = xQueueCreate(AppConfig::CommandQueueLength, sizeof(CommandMessage));
  if (g_commandQueue == nullptr) {
    haltBoot("failed to create command queue");
  }

  if (!startNetworkTask(g_systemEvents, g_commandQueue)) {
    haltBoot("failed to start NetworkTask");
  }

  if (!startUiTask(g_systemEvents, g_commandQueue)) {
    haltBoot("failed to start UiTask");
  }

  Serial.println("[boot] tasks started");
}

void loop() {
  vTaskDelete(nullptr);
}
