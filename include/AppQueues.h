#pragma once

#include <Arduino.h>

enum class CommandType : uint8_t {
  PublishMqttCommand = 0,
};

struct CommandMessage {
  CommandType type = CommandType::PublishMqttCommand;
  char topic[96] = {};
  char payload[160] = {};
  uint32_t createdAtMs = 0;
};
