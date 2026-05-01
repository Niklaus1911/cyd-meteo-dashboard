#include "AppRuntime.h"

EventGroupHandle_t g_systemEvents = nullptr;
QueueHandle_t g_commandQueue = nullptr;
QueueHandle_t g_mqttInboundQueue = nullptr;
