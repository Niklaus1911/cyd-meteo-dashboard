#include "ui/LvglPort.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "Log.h"
#include "display/TftDisplay.h"
#include "ui/TouchInput.h"

namespace {

constexpr uint16_t MaxDisplayWidth = 320;
constexpr uint32_t LvglTickMs = 5;

lv_disp_draw_buf_t s_drawBuffer;
lv_color_t s_buffer1[MaxDisplayWidth * AppConfig::LvglDrawBufferLines];
lv_disp_drv_t s_displayDriver;
uint32_t s_lastTickMs = 0;
uint32_t s_lastPerfLogMs = 0;
uint32_t s_handlerCalls = 0;
uint32_t s_flushCount = 0;
uint32_t s_handlerMaxUs = 0;
uint64_t s_handlerTotalUs = 0;

void flushDisplay(lv_disp_drv_t* displayDriver, const lv_area_t* area, lv_color_t* colorBuffer) {
  const uint32_t width = area->x2 - area->x1 + 1;
  const uint32_t height = area->y2 - area->y1 + 1;

  TFT_eSPI& tft = TftDisplay::tft();
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, width, height);
  tft.pushColors(reinterpret_cast<uint16_t*>(&colorBuffer->full), width * height, true);
  tft.endWrite();

  ++s_flushCount;
  lv_disp_flush_ready(displayDriver);
}

void logPerformance(uint32_t nowMs) {
  if (s_lastPerfLogMs == 0) {
    s_lastPerfLogMs = nowMs;
    return;
  }

  const uint32_t elapsedMs = nowMs - s_lastPerfLogMs;
  if (elapsedMs < AppConfig::LvglPerfLogPeriodMs) {
    return;
  }

  const uint32_t elapsedSec = elapsedMs / 1000UL;
  const uint32_t handlerRate = elapsedSec > 0 ? s_handlerCalls / elapsedSec : 0;
  const uint32_t flushRate = elapsedSec > 0 ? s_flushCount / elapsedSec : 0;
  const uint32_t handlerAvgUs =
      s_handlerCalls > 0 ? static_cast<uint32_t>(s_handlerTotalUs / s_handlerCalls) : 0;

  LOG_TASK("LVGL perf heap=%lu loop_hz=%lu handler_avg_us=%lu handler_max_us=%lu flush_hz=%lu draw_lines=%u",
           static_cast<unsigned long>(ESP.getFreeHeap()),
           static_cast<unsigned long>(handlerRate),
           static_cast<unsigned long>(handlerAvgUs),
           static_cast<unsigned long>(s_handlerMaxUs),
           static_cast<unsigned long>(flushRate),
           AppConfig::LvglDrawBufferLines);

  s_lastPerfLogMs = nowMs;
  s_handlerCalls = 0;
  s_flushCount = 0;
  s_handlerMaxUs = 0;
  s_handlerTotalUs = 0;
}

}  // namespace

namespace LvglPort {

void begin() {
  TftDisplay::begin();

  lv_init();

  TFT_eSPI& tft = TftDisplay::tft();
  lv_disp_draw_buf_init(&s_drawBuffer,
                        s_buffer1,
                        nullptr,
                        MaxDisplayWidth * AppConfig::LvglDrawBufferLines);

  lv_disp_drv_init(&s_displayDriver);
  s_displayDriver.hor_res = tft.width();
  s_displayDriver.ver_res = tft.height();
  s_displayDriver.flush_cb = flushDisplay;
  s_displayDriver.draw_buf = &s_drawBuffer;
  lv_disp_drv_register(&s_displayDriver);

  TouchInput::begin(tft.width(), tft.height());

  s_lastTickMs = millis();
  s_lastPerfLogMs = s_lastTickMs;
  LOG_TASK("LVGL initialized hor=%d ver=%d draw_lines=%u",
           tft.width(),
           tft.height(),
           AppConfig::LvglDrawBufferLines);
}

void tick() {
  const uint32_t nowMs = millis();
  if (nowMs - s_lastTickMs >= LvglTickMs) {
    lv_tick_inc(nowMs - s_lastTickMs);
    s_lastTickMs = nowMs;
  }
}

void handleTimers() {
  const uint32_t startUs = micros();
  lv_timer_handler();
  const uint32_t durationUs = micros() - startUs;

  ++s_handlerCalls;
  s_handlerTotalUs += durationUs;
  if (durationUs > s_handlerMaxUs) {
    s_handlerMaxUs = durationUs;
  }

  logPerformance(millis());
}

}  // namespace LvglPort
