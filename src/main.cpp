#include <Arduino.h>
#include <WiFiManager.h>

#include "AppConfig.h"
#include "AppQueues.h"
#include "AppRuntime.h"
#include "Log.h"
#include "app/AppStateStore.h"
#include "app/MqttSettings.h"
#include "tasks/NetworkTask.h"
#include "tasks/UiTask.h"

namespace {

void haltBoot(const char* reason) {
  Serial.printf("[boot] fatal: %s\n", reason);
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

bool isConfigResetButtonPressed() {
  if (AppConfig::ConfigResetButtonPin < 0) {
    return false;
  }

  const int level = digitalRead(AppConfig::ConfigResetButtonPin);
  return AppConfig::ConfigResetButtonActiveLow ? level == LOW : level == HIGH;
}

void checkBootConfigReset() {
  if (AppConfig::ConfigResetButtonPin < 0) {
    Serial.println("[boot] config reset button disabled; set AppConfig::ConfigResetButtonPin to enable");
    return;
  }

  pinMode(AppConfig::ConfigResetButtonPin,
          AppConfig::ConfigResetButtonActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);
  vTaskDelay(pdMS_TO_TICKS(20));

  if (!isConfigResetButtonPressed()) {
    return;
  }

  Serial.printf("[boot] config reset button detected on GPIO %d; hold for %lu ms to reset\n",
                AppConfig::ConfigResetButtonPin,
                static_cast<unsigned long>(AppConfig::ConfigResetHoldMs));

  const uint32_t startedAtMs = millis();
  while (millis() - startedAtMs < AppConfig::ConfigResetHoldMs) {
    if (!isConfigResetButtonPressed()) {
      Serial.println("[boot] config reset cancelled; button released");
      return;
    }

    vTaskDelay(pdMS_TO_TICKS(AppConfig::ConfigResetSampleMs));
  }

  Serial.println("[boot] clearing WiFiManager WiFi credentials and MQTT settings");

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(false);
  wifiManager.resetSettings();

  if (MqttSettingsStore::clear()) {
    Serial.println("[boot] MQTT settings cleared from NVS");
  } else {
    Serial.println("[boot] failed to clear MQTT settings from NVS");
  }

  Serial.println("[boot] restarting after configuration reset");
  vTaskDelay(pdMS_TO_TICKS(AppConfig::RestartAfterConfigResetMs));
  ESP.restart();
}

}  // namespace

void setup() {
  Serial.begin(AppConfig::SerialBaud);
  vTaskDelay(pdMS_TO_TICKS(200));

  Serial.printf("\n[boot] CYD dashboard firmware skeleton starting on core %d\n", xPortGetCoreID());
  checkBootConfigReset();

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

  g_mqttInboundQueue = xQueueCreate(AppConfig::MqttInboundQueueLength, sizeof(MqttInboundMessage));
  if (g_mqttInboundQueue == nullptr) {
    haltBoot("failed to create MQTT inbound queue");
  }

  if (!startNetworkTask(g_systemEvents, g_commandQueue, g_mqttInboundQueue)) {
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
