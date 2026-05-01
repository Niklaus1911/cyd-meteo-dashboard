#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>

extern EventGroupHandle_t g_systemEvents;
extern QueueHandle_t g_commandQueue;
extern QueueHandle_t g_mqttInboundQueue;
