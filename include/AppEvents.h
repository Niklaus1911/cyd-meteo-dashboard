#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace AppEvents {

constexpr EventBits_t WifiConnected = BIT0;
constexpr EventBits_t MqttConnected = BIT1;
constexpr EventBits_t AppStateUpdated = BIT2;

}  // namespace AppEvents
