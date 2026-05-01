#include "ui/DashboardScreen.h"

#include <Arduino.h>
#include <lvgl.h>

namespace {

struct MetricCard {
  lv_obj_t* card = nullptr;
  lv_obj_t* title = nullptr;
  lv_obj_t* value = nullptr;
  lv_obj_t* detail = nullptr;
  lv_obj_t* bar = nullptr;
};

constexpr lv_coord_t CardPad = 8;

constexpr uint32_t ColorBg = 0x080D14;
constexpr uint32_t ColorPanel = 0x111A24;
constexpr uint32_t ColorPanelAlt = 0x142030;
constexpr uint32_t ColorBorder = 0x253344;
constexpr uint32_t ColorText = 0xF4F7FB;
constexpr uint32_t ColorMuted = 0x93A4B8;
constexpr uint32_t ColorGreen = 0x38D07A;
constexpr uint32_t ColorRed = 0xE35D6A;
constexpr uint32_t ColorAmber = 0xF4B740;
constexpr uint32_t ColorBlue = 0x4DA3FF;
constexpr uint32_t ColorCyan = 0x37D5D6;
constexpr uint32_t ColorPurple = 0xB58BFF;

lv_obj_t* s_title = nullptr;
lv_obj_t* s_wifiBadge = nullptr;
lv_obj_t* s_mqttBadge = nullptr;
lv_obj_t* s_statusBadge = nullptr;
lv_obj_t* s_age = nullptr;
lv_obj_t* s_footer = nullptr;

MetricCard s_temperature;
MetricCard s_humidity;
MetricCard s_pressure;
MetricCard s_battery;
MetricCard s_solar;

lv_obj_t* createLabel(lv_obj_t* parent,
                      const char* text,
                      const lv_font_t* font,
                      uint32_t color,
                      lv_coord_t x,
                      lv_coord_t y,
                      lv_coord_t width) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_width(label, width);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  return label;
}

lv_obj_t* createCard(lv_obj_t* parent,
                     lv_coord_t x,
                     lv_coord_t y,
                     lv_coord_t width,
                     lv_coord_t height,
                     uint32_t color = ColorPanel) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, width, height);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(card, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(ColorBorder), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  return card;
}

lv_obj_t* createBadge(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t width) {
  lv_obj_t* badge = lv_label_create(parent);
  lv_obj_set_pos(badge, x, y);
  lv_obj_set_size(badge, width, 22);
  lv_label_set_long_mode(badge, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_radius(badge, 6, 0);
  lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_top(badge, 4, 0);
  lv_obj_set_style_text_align(badge, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(badge, &lv_font_montserrat_12, 0);
  return badge;
}

MetricCard createMetricCard(lv_obj_t* parent,
                            const char* title,
                            lv_coord_t x,
                            lv_coord_t y,
                            lv_coord_t width,
                            lv_coord_t height,
                            bool large,
                            bool compact,
                            uint32_t accent) {
  MetricCard metric;
  metric.card = createCard(parent, x, y, width, height, large ? ColorPanelAlt : ColorPanel);
  metric.title = createLabel(metric.card,
                             title,
                             &lv_font_montserrat_12,
                             ColorMuted,
                             CardPad,
                             7,
                             width - (CardPad * 2));
  metric.value = createLabel(metric.card,
                             "--",
                             large ? &lv_font_montserrat_20 :
                                     (compact ? &lv_font_montserrat_14 : &lv_font_montserrat_16),
                             accent,
                             CardPad,
                             large ? 31 : 29,
                             width - (CardPad * 2));
  metric.detail = createLabel(metric.card,
                              "",
                              &lv_font_montserrat_12,
                              ColorMuted,
                              CardPad,
                              height - 20,
                              width - (CardPad * 2));
  return metric;
}

void setLabel(lv_obj_t* label, const char* text) {
  if (label != nullptr) {
    lv_label_set_text(label, text);
  }
}

void setBadge(lv_obj_t* badge, const char* text, uint32_t bgColor, uint32_t textColor = ColorText) {
  if (badge == nullptr) {
    return;
  }

  lv_label_set_text(badge, text);
  lv_obj_set_style_bg_color(badge, lv_color_hex(bgColor), 0);
  lv_obj_set_style_text_color(badge, lv_color_hex(textColor), 0);
}

void formatMetricValue(char* buffer,
                       size_t bufferSize,
                       float value,
                       bool valid,
                       const char* suffix,
                       uint8_t decimals) {
  if (!valid || isnan(value)) {
    snprintf(buffer, bufferSize, "-- %s", suffix);
    return;
  }

  if (decimals == 0) {
    snprintf(buffer, bufferSize, "%.0f %s", value, suffix);
  } else if (decimals == 2) {
    snprintf(buffer, bufferSize, "%.2f %s", value, suffix);
  } else {
    snprintf(buffer, bufferSize, "%.1f %s", value, suffix);
  }
}

uint32_t lastUpdateAgeMs(const AppState& state) {
  if (state.lastMqttReceiveMs == 0 || state.uptimeMs < state.lastMqttReceiveMs) {
    return 0;
  }

  return static_cast<uint32_t>(state.uptimeMs - state.lastMqttReceiveMs);
}

void updateBatteryBar(const AppState& state) {
  if (s_battery.bar == nullptr) {
    return;
  }

  int32_t percent = 0;
  if (state.latestSensor.batteryPercentValid && !isnan(state.latestSensor.batteryPercent)) {
    percent = static_cast<int32_t>(state.latestSensor.batteryPercent + 0.5f);
    percent = constrain(percent, 0, 100);
  }

  lv_bar_set_value(s_battery.bar, percent, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(s_battery.bar,
                            lv_color_hex(percent < 20 ? ColorRed : ColorGreen),
                            LV_PART_INDICATOR);
}

}  // namespace

namespace DashboardScreen {

void create() {
  lv_obj_t* screen = lv_scr_act();
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen, lv_color_hex(ColorBg), 0);
  lv_obj_set_style_text_color(screen, lv_color_hex(ColorText), 0);

  s_title = createLabel(screen,
                        "Meteo Dashboard",
                        &lv_font_montserrat_16,
                        ColorText,
                        8,
                        8,
                        158);

  s_wifiBadge = createBadge(screen, 172, 6, 66);
  s_mqttBadge = createBadge(screen, 244, 6, 68);
  s_statusBadge = createBadge(screen, 8, 34, 84);

  s_age = createLabel(screen,
                      "last update --",
                      &lv_font_montserrat_12,
                      ColorMuted,
                      100,
                      38,
                      210);
  lv_obj_set_style_text_align(s_age, LV_TEXT_ALIGN_RIGHT, 0);

  s_temperature = createMetricCard(screen, "TEMP", 8, 60, 152, 82, true, false, ColorCyan);
  s_solar = createMetricCard(screen, "SOLAR", 8, 148, 152, 62, false, false, ColorAmber);
  s_humidity = createMetricCard(screen, "HUMID", 166, 60, 70, 62, false, true, ColorBlue);
  s_pressure = createMetricCard(screen, "PRESS", 242, 60, 70, 62, false, true, ColorPurple);
  s_battery = createMetricCard(screen, "BATTERY", 166, 130, 146, 80, false, false, ColorGreen);

  lv_obj_set_pos(s_solar.detail, CardPad, 43);
  lv_obj_set_pos(s_battery.detail, CardPad, 50);

  s_battery.bar = lv_bar_create(s_battery.card);
  lv_obj_set_pos(s_battery.bar, CardPad, 64);
  lv_obj_set_size(s_battery.bar, 130, 6);
  lv_bar_set_range(s_battery.bar, 0, 100);
  lv_obj_set_style_bg_color(s_battery.bar, lv_color_hex(0x263241), 0);
  lv_obj_set_style_bg_color(s_battery.bar, lv_color_hex(ColorGreen), LV_PART_INDICATOR);
  lv_obj_set_style_radius(s_battery.bar, 3, 0);
  lv_obj_set_style_radius(s_battery.bar, 3, LV_PART_INDICATOR);

  s_footer = createLabel(screen,
                         "uptime --",
                         &lv_font_montserrat_12,
                         ColorMuted,
                         8,
                         220,
                         304);
  lv_obj_set_style_text_align(s_footer, LV_TEXT_ALIGN_CENTER, 0);

  setBadge(s_wifiBadge, "WiFi --", 0x233041, ColorMuted);
  setBadge(s_mqttBadge, "MQTT --", 0x233041, ColorMuted);
  setBadge(s_statusBadge, "NO DATA", 0x233041, ColorMuted);
}

void update(const AppState& state) {
  char buffer[80] = {};

  setBadge(s_wifiBadge,
           state.wifiConnected ? "WiFi OK" : "WiFi --",
           state.wifiConnected ? 0x1B6B3B : 0x4A2530,
           state.wifiConnected ? ColorText : ColorMuted);

  setBadge(s_mqttBadge,
           state.mqttConnected ? "MQTT OK" : "MQTT --",
           state.mqttConnected ? 0x1B6B3B : 0x4A2530,
           state.mqttConnected ? ColorText : ColorMuted);

  const bool hasData = state.latestSensor.valid;
  if (!hasData) {
    setBadge(s_statusBadge, "NO DATA", 0x263241, ColorMuted);
  } else if (state.latestSensor.stale) {
    setBadge(s_statusBadge, "STALE", 0x6F4B12, ColorText);
  } else {
    setBadge(s_statusBadge, "LIVE", 0x1B6B3B, ColorText);
  }

  const uint32_t ageMs = lastUpdateAgeMs(state);
  if (state.lastMqttReceiveMs == 0) {
    snprintf(buffer, sizeof(buffer), "last update --");
  } else {
    snprintf(buffer, sizeof(buffer), "last update %lus", static_cast<unsigned long>(ageMs / 1000UL));
  }
  setLabel(s_age, buffer);

  formatMetricValue(buffer,
                    sizeof(buffer),
                    state.latestSensor.outsideTemperatureC,
                    state.latestSensor.outsideTemperatureValid,
                    "C",
                    1);
  setLabel(s_temperature.value, buffer);
  setLabel(s_temperature.detail, state.latestSensor.stale ? "stale" : "");

  if (state.latestSensor.outsideHumidityValid && !isnan(state.latestSensor.outsideHumidityPercent)) {
    snprintf(buffer, sizeof(buffer), "%.0f%%", state.latestSensor.outsideHumidityPercent);
  } else {
    snprintf(buffer, sizeof(buffer), "--%%");
  }
  setLabel(s_humidity.value, buffer);
  setLabel(s_humidity.detail, "");

  if (state.latestSensor.absolutePressureValid && !isnan(state.latestSensor.absolutePressurehPa)) {
    snprintf(buffer, sizeof(buffer), "%.0f", state.latestSensor.absolutePressurehPa);
  } else {
    snprintf(buffer, sizeof(buffer), "--");
  }
  setLabel(s_pressure.value, buffer);
  setLabel(s_pressure.detail, "hPa");

  if (state.latestSensor.batteryPercentValid && !isnan(state.latestSensor.batteryPercent)) {
    snprintf(buffer, sizeof(buffer), "%.0f%%", state.latestSensor.batteryPercent);
  } else {
    snprintf(buffer, sizeof(buffer), "--%%");
  }
  setLabel(s_battery.value, buffer);
  setLabel(s_battery.detail, state.latestSensor.batteryPercentValid ? "" : "waiting");
  updateBatteryBar(state);

  formatMetricValue(buffer,
                    sizeof(buffer),
                    state.latestSensor.solarPanelVoltageV,
                    state.latestSensor.solarPanelVoltageValid,
                    "V",
                    2);
  setLabel(s_solar.value, buffer);

  if (state.latestSensor.solarPanelCurrentValid && !isnan(state.latestSensor.solarPanelCurrentmA)) {
    snprintf(buffer, sizeof(buffer), "%.1f mA", state.latestSensor.solarPanelCurrentmA);
  } else {
    snprintf(buffer, sizeof(buffer), "-- mA");
  }
  setLabel(s_solar.detail, buffer);

  snprintf(buffer,
           sizeof(buffer),
           "uptime %lus",
           static_cast<unsigned long>(state.uptimeMs / 1000UL));
  setLabel(s_footer, buffer);
}

}  // namespace DashboardScreen
