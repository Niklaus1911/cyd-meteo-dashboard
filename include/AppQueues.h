#pragma once

#include <Arduino.h>

#include "AppConfig.h"

enum class CommandType : uint8_t {
  PublishMqttCommand = 0,
  ResetCredentials,
};

struct CommandMessage {
  CommandType type = CommandType::PublishMqttCommand;
  char topic[96] = {};
  char payload[160] = {};
  uint32_t createdAtMs = 0;
};

struct MqttInboundMessage {
  char topic[AppConfig::MqttTopicMaxLength] = {};
  char payload[AppConfig::MqttPayloadMaxLength] = {};
  uint32_t payloadLength = 0;
  uint32_t receivedAtMs = 0;
};
