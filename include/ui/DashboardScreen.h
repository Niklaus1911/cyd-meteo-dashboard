#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "AppState.h"

namespace DashboardScreen {

void create(QueueHandle_t commandQueue);
void update(const AppState& state);

}  // namespace DashboardScreen
