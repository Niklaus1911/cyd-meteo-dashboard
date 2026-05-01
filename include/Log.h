#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LOG_TASK(format, ...)                                                         \
  do {                                                                                \
    Serial.printf("[%10lu ms][%-12s][core %d] " format "\n",                         \
                  static_cast<unsigned long>(millis()),                               \
                  pcTaskGetName(nullptr),                                             \
                  xPortGetCoreID(),                                                   \
                  ##__VA_ARGS__);                                                     \
  } while (false)
