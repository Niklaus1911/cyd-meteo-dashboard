#pragma once

#include <Arduino.h>

#include "AppConfig.h"

struct MqttSettings {
  char host[AppConfig::MqttHostMaxLength] = {};
  uint16_t port = AppConfig::DefaultMqttPort;
  char username[AppConfig::MqttUserMaxLength] = {};
  char password[AppConfig::MqttPasswordMaxLength] = {};

  bool isComplete() const;
  bool hasCredentials() const;
};

namespace MqttSettingsStore {

bool load(MqttSettings& settings);
bool save(const MqttSettings& settings);
bool clear();

}  // namespace MqttSettingsStore
