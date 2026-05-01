#include "ui/LvglPort.h"

#include <Arduino.h>

#include "Log.h"
#include "display/TftDisplay.h"

namespace {

constexpr uint16_t DrawBufferLines = 10;
constexpr uint16_t MaxDisplayWidth = 320;
constexpr uint32_t LvglTickMs = 5;

lv_disp_draw_buf_t s_drawBuffer;
lv_color_t s_buffer1[MaxDisplayWidth * DrawBufferLines];
lv_disp_drv_t s_displayDriver;
uint32_t s_lastTickMs = 0;

void flushDisplay(lv_disp_drv_t* displayDriver, const lv_area_t* area, lv_color_t* colorBuffer) {
  const uint32_t width = area->x2 - area->x1 + 1;
  const uint32_t height = area->y2 - area->y1 + 1;

  TFT_eSPI& tft = TftDisplay::tft();
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, width, height);
  tft.pushColors(reinterpret_cast<uint16_t*>(&colorBuffer->full), width * height, true);
  tft.endWrite();

  lv_disp_flush_ready(displayDriver);
}

}  // namespace

namespace LvglPort {

void begin() {
  TftDisplay::begin();

  lv_init();

  TFT_eSPI& tft = TftDisplay::tft();
  lv_disp_draw_buf_init(&s_drawBuffer, s_buffer1, nullptr, MaxDisplayWidth * DrawBufferLines);

  lv_disp_drv_init(&s_displayDriver);
  s_displayDriver.hor_res = tft.width();
  s_displayDriver.ver_res = tft.height();
  s_displayDriver.flush_cb = flushDisplay;
  s_displayDriver.draw_buf = &s_drawBuffer;
  lv_disp_drv_register(&s_displayDriver);

  s_lastTickMs = millis();
  LOG_TASK("LVGL initialized hor=%d ver=%d draw_lines=%u", tft.width(), tft.height(), DrawBufferLines);
}

void tick() {
  const uint32_t nowMs = millis();
  if (nowMs - s_lastTickMs >= LvglTickMs) {
    lv_tick_inc(nowMs - s_lastTickMs);
    s_lastTickMs = nowMs;
  }
}

void handleTimers() {
  lv_timer_handler();
}

}  // namespace LvglPort
