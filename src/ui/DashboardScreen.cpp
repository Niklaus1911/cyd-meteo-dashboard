#include "ui/DashboardScreen.h"

#include <Arduino.h>
#include <lvgl.h>

#include "AppQueues.h"

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

enum class Page : uint8_t {
  Dashboard,
  Settings,
  ResetConfirm,
  Resetting,
};

QueueHandle_t s_commandQueue = nullptr;

lv_obj_t* s_dashboardPage = nullptr;
lv_obj_t* s_settingsPage = nullptr;
lv_obj_t* s_confirmPage = nullptr;
lv_obj_t* s_resettingPage = nullptr;

lv_obj_t* s_wifiBadge = nullptr;
lv_obj_t* s_mqttBadge = nullptr;
lv_obj_t* s_statusBadge = nullptr;
lv_obj_t* s_age = nullptr;
lv_obj_t* s_footer = nullptr;
lv_obj_t* s_gearButton = nullptr;

lv_obj_t* s_settingsWifi = nullptr;
lv_obj_t* s_settingsMqtt = nullptr;
lv_obj_t* s_settingsBroker = nullptr;
lv_obj_t* s_eraseButton = nullptr;
lv_obj_t* s_resettingTitle = nullptr;
lv_obj_t* s_resettingDetail = nullptr;

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

lv_obj_t* createPage(lv_obj_t* parent) {
  lv_obj_t* page = lv_obj_create(parent);
  lv_obj_set_pos(page, 0, 0);
  lv_obj_set_size(page, 320, 240);
  lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(page, lv_color_hex(ColorBg), 0);
  lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(page, 0, 0);
  lv_obj_set_style_radius(page, 0, 0);
  lv_obj_set_style_pad_all(page, 0, 0);
  lv_obj_set_style_text_color(page, lv_color_hex(ColorText), 0);
  return page;
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
  lv_obj_set_size(badge, width, 20);
  lv_label_set_long_mode(badge, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_radius(badge, 5, 0);
  lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_top(badge, 3, 0);
  lv_obj_set_style_text_align(badge, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(badge, &lv_font_montserrat_12, 0);
  return badge;
}

void styleButton(lv_obj_t* button, uint32_t color = ColorPanelAlt) {
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(button, lv_color_hex(ColorBorder), 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_radius(button, 7, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_pad_all(button, 0, 0);
}

lv_obj_t* createButton(lv_obj_t* parent,
                       const char* text,
                       lv_coord_t x,
                       lv_coord_t y,
                       lv_coord_t width,
                       lv_coord_t height,
                       lv_event_cb_t callback,
                       uint32_t color = ColorPanelAlt) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_size(button, width, height);
  styleButton(button, color);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(label, width - 8);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(ColorText), 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(label);
  return button;
}

MetricCard createMetricCard(lv_obj_t* parent,
                            const char* title,
                            lv_coord_t x,
                            lv_coord_t y,
                            lv_coord_t width,
                            lv_coord_t height,
                            bool large,
                            uint32_t accent) {
  MetricCard metric;
  metric.card = createCard(parent, x, y, width, height);
  metric.title = createLabel(metric.card,
                             title,
                             &lv_font_montserrat_12,
                             ColorMuted,
                             CardPad,
                             7,
                             width - (CardPad * 2));
  metric.value = createLabel(metric.card,
                             "--",
                             large ? &lv_font_montserrat_20 : &lv_font_montserrat_16,
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

void showPage(Page page) {
  if (s_dashboardPage != nullptr) {
    lv_obj_add_flag(s_dashboardPage, LV_OBJ_FLAG_HIDDEN);
  }
  if (s_settingsPage != nullptr) {
    lv_obj_add_flag(s_settingsPage, LV_OBJ_FLAG_HIDDEN);
  }
  if (s_confirmPage != nullptr) {
    lv_obj_add_flag(s_confirmPage, LV_OBJ_FLAG_HIDDEN);
  }
  if (s_resettingPage != nullptr) {
    lv_obj_add_flag(s_resettingPage, LV_OBJ_FLAG_HIDDEN);
  }

  lv_obj_t* visiblePage = nullptr;
  switch (page) {
    case Page::Dashboard:
      visiblePage = s_dashboardPage;
      break;
    case Page::Settings:
      visiblePage = s_settingsPage;
      break;
    case Page::ResetConfirm:
      visiblePage = s_confirmPage;
      break;
    case Page::Resetting:
      visiblePage = s_resettingPage;
      break;
  }

  if (visiblePage != nullptr) {
    lv_obj_clear_flag(visiblePage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(visiblePage);
  }
}

void sendResetCredentialsCommand() {
  if (s_commandQueue == nullptr) {
    return;
  }

  CommandMessage command;
  command.type = CommandType::ResetCredentials;
  command.createdAtMs = millis();
  xQueueSend(s_commandQueue, &command, 0);
}

void onGearButton(lv_event_t*) {
  showPage(Page::Settings);
}

void onBackButton(lv_event_t*) {
  showPage(Page::Dashboard);
}

void onResetButton(lv_event_t*) {
  showPage(Page::ResetConfirm);
}

void onCancelButton(lv_event_t*) {
  showPage(Page::Settings);
}

void onEraseButton(lv_event_t*) {
  if (s_eraseButton != nullptr) {
    lv_obj_add_state(s_eraseButton, LV_STATE_DISABLED);
  }

  setLabel(s_resettingTitle, "Resetting...");
  setLabel(s_resettingDetail, "Rebooting to setup mode");
  showPage(Page::Resetting);
  sendResetCredentialsCommand();
}

uint32_t lastUpdateAgeMs(const AppState& state) {
  if (state.lastMqttReceiveMs == 0 || state.uptimeMs < state.lastMqttReceiveMs) {
    return 0;
  }

  return static_cast<uint32_t>(state.uptimeMs - state.lastMqttReceiveMs);
}

void formatDuration(char* buffer,
                    size_t bufferSize,
                    const char* prefix,
                    uint32_t durationMs,
                    bool includeHours) {
  const uint32_t totalSeconds = durationMs / 1000UL;
  const uint32_t seconds = totalSeconds % 60UL;
  const uint32_t totalMinutes = totalSeconds / 60UL;

  if (includeHours && totalMinutes >= 60UL) {
    snprintf(buffer,
             bufferSize,
             "%s %luh %02lum",
             prefix,
             static_cast<unsigned long>(totalMinutes / 60UL),
             static_cast<unsigned long>(totalMinutes % 60UL));
  } else if (totalMinutes > 0) {
    snprintf(buffer,
             bufferSize,
             "%s %lum %02lus",
             prefix,
             static_cast<unsigned long>(totalMinutes),
             static_cast<unsigned long>(seconds));
  } else {
    snprintf(buffer, bufferSize, "%s %lus", prefix, static_cast<unsigned long>(seconds));
  }
}

void updateBatteryBar(const AppState& state) {
  if (s_battery.bar == nullptr) {
    return;
  }

  int32_t percent = 0;
  const bool valid = state.latestSensor.batteryPercentValid && !isnan(state.latestSensor.batteryPercent);
  if (valid) {
    percent = static_cast<int32_t>(state.latestSensor.batteryPercent + 0.5f);
    percent = constrain(percent, 0, 100);
  }

  lv_bar_set_value(s_battery.bar, percent, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(s_battery.bar,
                            lv_color_hex(valid ? (percent < 20 ? ColorRed : ColorGreen) : 0x263241),
                            LV_PART_INDICATOR);
}

}  // namespace

namespace DashboardScreen {

void create(QueueHandle_t commandQueue) {
  s_commandQueue = commandQueue;

  lv_obj_t* screen = lv_scr_act();
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen, lv_color_hex(ColorBg), 0);
  lv_obj_set_style_text_color(screen, lv_color_hex(ColorText), 0);

  s_dashboardPage = createPage(screen);
  s_settingsPage = createPage(screen);
  s_confirmPage = createPage(screen);
  s_resettingPage = createPage(screen);

  createLabel(s_dashboardPage, "ESP Meteo", &lv_font_montserrat_16, ColorText, 8, 6, 88);

  s_wifiBadge = createBadge(s_dashboardPage, 104, 5, 72);
  s_mqttBadge = createBadge(s_dashboardPage, 182, 5, 94);
  s_gearButton = createButton(s_dashboardPage,
#if defined(LV_SYMBOL_SETTINGS)
                              LV_SYMBOL_SETTINGS,
#else
                              "SET",
#endif
                              284,
                              2,
                              28,
                              28,
                              onGearButton,
                              ColorPanel);
  lv_obj_set_style_text_font(lv_obj_get_child(s_gearButton, 0), &lv_font_montserrat_16, 0);

  s_statusBadge = createBadge(s_dashboardPage, 8, 29, 76);

  s_age = createLabel(s_dashboardPage,
                      "age --",
                      &lv_font_montserrat_12,
                      ColorMuted,
                      94,
                      33,
                      116);

  s_temperature = createMetricCard(s_dashboardPage, "TEMP", 8, 53, 118, 68, true, ColorCyan);
  s_humidity = createMetricCard(s_dashboardPage, "HUMID", 132, 53, 82, 68, false, ColorBlue);
  s_pressure = createMetricCard(s_dashboardPage, "PRESS", 220, 53, 92, 68, false, ColorPurple);
  s_solar = createMetricCard(s_dashboardPage, "SOLAR", 8, 130, 190, 96, false, ColorAmber);
  s_battery = createMetricCard(s_dashboardPage, "BATTERY", 204, 130, 108, 96, false, ColorGreen);

  lv_obj_set_width(s_temperature.value, 102);
  lv_obj_set_pos(s_temperature.value, CardPad, 28);
  lv_obj_set_width(s_temperature.detail, 102);
  lv_obj_set_pos(s_temperature.detail, CardPad, 52);

  lv_obj_set_width(s_humidity.value, 66);
  lv_obj_set_pos(s_humidity.value, CardPad, 31);

  lv_obj_set_width(s_pressure.value, 76);
  lv_obj_set_pos(s_pressure.value, CardPad, 31);
  lv_obj_set_width(s_pressure.detail, 76);
  lv_obj_set_pos(s_pressure.detail, CardPad, 52);

  lv_obj_set_width(s_solar.value, 174);
  lv_obj_set_pos(s_solar.value, CardPad, 31);
  lv_obj_set_style_text_font(s_solar.value, &lv_font_montserrat_20, 0);
  lv_obj_set_pos(s_solar.detail, CardPad, 61);
  lv_obj_set_width(s_solar.detail, 174);
  lv_obj_set_style_text_font(s_solar.detail, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(s_solar.detail, lv_color_hex(ColorAmber), 0);

  lv_obj_set_pos(s_battery.value, CardPad, 32);
  lv_obj_set_width(s_battery.value, 92);
  lv_obj_set_pos(s_battery.detail, CardPad, 74);
  lv_obj_set_width(s_battery.detail, 92);

  s_battery.bar = lv_bar_create(s_battery.card);
  lv_obj_set_pos(s_battery.bar, CardPad, 58);
  lv_obj_set_size(s_battery.bar, 92, 7);
  lv_bar_set_range(s_battery.bar, 0, 100);
  lv_obj_set_style_bg_color(s_battery.bar, lv_color_hex(0x263241), 0);
  lv_obj_set_style_bg_color(s_battery.bar, lv_color_hex(ColorGreen), LV_PART_INDICATOR);
  lv_obj_set_style_radius(s_battery.bar, 3, 0);
  lv_obj_set_style_radius(s_battery.bar, 3, LV_PART_INDICATOR);

  s_footer = createLabel(s_dashboardPage,
                         "uptime --",
                         &lv_font_montserrat_12,
                         ColorMuted,
                         218,
                         33,
                         94);
  lv_obj_set_style_text_align(s_footer, LV_TEXT_ALIGN_RIGHT, 0);

  setBadge(s_wifiBadge, "WiFi --", 0x233041, ColorMuted);
  setBadge(s_mqttBadge, "MQTT --", 0x233041, ColorMuted);
  setBadge(s_statusBadge, "NO DATA", 0x233041, ColorMuted);

  createLabel(s_settingsPage, "Settings", &lv_font_montserrat_20, ColorText, 8, 10, 160);
  createButton(s_settingsPage, "BACK", 236, 8, 76, 30, onBackButton, ColorPanel);
  s_settingsWifi = createLabel(s_settingsPage, "WiFi: --", &lv_font_montserrat_14, ColorText, 14, 58, 292);
  s_settingsMqtt = createLabel(s_settingsPage, "MQTT: --", &lv_font_montserrat_14, ColorText, 14, 86, 292);
  s_settingsBroker = createLabel(s_settingsPage,
                                 "Broker: --",
                                 &lv_font_montserrat_14,
                                 ColorMuted,
                                 14,
                                 114,
                                 292);
  createButton(s_settingsPage, "Reset WiFi/MQTT", 44, 166, 232, 42, onResetButton, 0x243246);

  createLabel(s_confirmPage,
              "Erase saved WiFi and MQTT settings?",
              &lv_font_montserrat_16,
              ColorText,
              18,
              54,
              284);
  createLabel(s_confirmPage,
              "The device will reboot to setup mode.",
              &lv_font_montserrat_12,
              ColorMuted,
              34,
              92,
              252);
  createButton(s_confirmPage, "Cancel", 42, 154, 104, 42, onCancelButton, ColorPanel);
  s_eraseButton = createButton(s_confirmPage, "Erase", 174, 154, 104, 42, onEraseButton, 0x5A2530);

  s_resettingTitle = createLabel(s_resettingPage,
                                 "Resetting...",
                                 &lv_font_montserrat_20,
                                 ColorText,
                                 48,
                                 84,
                                 224);
  lv_obj_set_style_text_align(s_resettingTitle, LV_TEXT_ALIGN_CENTER, 0);
  s_resettingDetail = createLabel(s_resettingPage,
                                  "Rebooting to setup mode",
                                  &lv_font_montserrat_14,
                                  ColorMuted,
                                  34,
                                  124,
                                  252);
  lv_obj_set_style_text_align(s_resettingDetail, LV_TEXT_ALIGN_CENTER, 0);

  showPage(Page::Dashboard);
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

  snprintf(buffer,
           sizeof(buffer),
           "WiFi: %s",
           state.wifiConnected ? state.wifiSsid : "--");
  setLabel(s_settingsWifi, buffer);

  snprintf(buffer,
           sizeof(buffer),
           "MQTT: %s",
           state.mqttConnected ? "connected" : "disconnected");
  setLabel(s_settingsMqtt, buffer);

  snprintf(buffer,
           sizeof(buffer),
           "Broker: %s",
           state.mqttBrokerHost[0] != '\0' ? state.mqttBrokerHost : "--");
  setLabel(s_settingsBroker, buffer);

  if (state.credentialResetRequested || state.credentialResetting || state.credentialRebooting) {
    setLabel(s_resettingTitle, state.credentialResetting ? "Resetting..." : "Reset requested");
    setLabel(s_resettingDetail,
             state.credentialRebooting ? "Rebooting to setup mode" : "Preparing reboot...");
    showPage(Page::Resetting);
  }

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
    snprintf(buffer, sizeof(buffer), "age --");
  } else {
    formatDuration(buffer, sizeof(buffer), "age", ageMs, false);
  }
  setLabel(s_age, buffer);

  if (state.latestSensor.outsideTemperatureValid && !isnan(state.latestSensor.outsideTemperatureC)) {
    snprintf(buffer,
             sizeof(buffer),
             "%.1f \xC2\xB0"
             "C",
             state.latestSensor.outsideTemperatureC);
  } else {
    snprintf(buffer,
             sizeof(buffer),
             "--.- \xC2\xB0"
             "C");
  }
  setLabel(s_temperature.value, buffer);
  setLabel(s_temperature.detail, "");

  if (state.latestSensor.outsideHumidityValid && !isnan(state.latestSensor.outsideHumidityPercent)) {
    snprintf(buffer, sizeof(buffer), "%.0f%%", state.latestSensor.outsideHumidityPercent);
  } else {
    snprintf(buffer, sizeof(buffer), "--%%");
  }
  setLabel(s_humidity.value, buffer);
  setLabel(s_humidity.detail, "");

  if (state.latestSensor.absolutePressureValid && !isnan(state.latestSensor.absolutePressurehPa)) {
    snprintf(buffer, sizeof(buffer), "%.2f", state.latestSensor.absolutePressurehPa);
  } else {
    snprintf(buffer, sizeof(buffer), "----.--");
  }
  setLabel(s_pressure.value, buffer);
  setLabel(s_pressure.detail, "hPa");

  if (state.latestSensor.batteryPercentValid && !isnan(state.latestSensor.batteryPercent)) {
    snprintf(buffer, sizeof(buffer), "%.1f%%", state.latestSensor.batteryPercent);
  } else {
    snprintf(buffer, sizeof(buffer), "--.-%%");
  }
  setLabel(s_battery.value, buffer);
  setLabel(s_battery.detail, "");
  updateBatteryBar(state);

  if (state.latestSensor.solarPanelVoltageValid && !isnan(state.latestSensor.solarPanelVoltageV)) {
    snprintf(buffer, sizeof(buffer), "%.3f V", state.latestSensor.solarPanelVoltageV);
  } else {
    snprintf(buffer, sizeof(buffer), "-.--- V");
  }
  setLabel(s_solar.value, buffer);

  if (state.latestSensor.solarPanelCurrentValid && !isnan(state.latestSensor.solarPanelCurrentmA)) {
    snprintf(buffer, sizeof(buffer), "%.3f mA", state.latestSensor.solarPanelCurrentmA);
  } else {
    snprintf(buffer, sizeof(buffer), "-.--- mA");
  }
  setLabel(s_solar.detail, buffer);

  formatDuration(buffer, sizeof(buffer), "up", state.uptimeMs, true);
  setLabel(s_footer, buffer);
}

}  // namespace DashboardScreen
