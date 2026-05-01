#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>

bool startNetworkTask(EventGroupHandle_t systemEvents, QueueHandle_t commandQueue);
