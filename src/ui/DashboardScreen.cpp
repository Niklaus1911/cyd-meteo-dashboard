#include "ui/DashboardScreen.h"

#include <Arduino.h>
#include <lvgl.h>

namespace {

lv_obj_t* s_title = nullptr;
lv_obj_t* s_wifi = nullptr;
lv_obj_t* s_mqtt = nullptr;
lv_obj_t* s_telemetry = nullptr;
lv_obj_t* s_temperature = nullptr;
lv_obj_t* s_humidity = nullptr;
lv_obj_t* s_pressure = nullptr;
lv_obj_t* s_battery = nullptr;
lv_obj_t* s_solarVoltage = nullptr;
lv_obj_t* s_solarCurrent = nullptr;
lv_obj_t* s_footer = nullptr;

void setLabel(lv_obj_t* label, const char* text) {
  if (label != nullptr) {
    lv_label_set_text(label, text);
  }
}

const char* yesNo(bool value) {
  return value ? "yes" : "no";
}

void formatFloatValue(char* buffer,
                      size_t bufferSize,
                      const char* label,
                      float value,
                      bool valid,
                      const char* suffix,
                      uint8_t decimals = 1) {
  if (!valid || isnan(value)) {
    snprintf(buffer, bufferSize, "%s --", label);
    return;
  }

  snprintf(buffer, bufferSize, decimals == 0 ? "%s %.0f %s" : "%s %.1f %s", label, value, suffix);
}

}  // namespace

namespace DashboardScreen {

void create() {
  lv_obj_t* screen = lv_scr_act();
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), 0);
  lv_obj_set_style_text_color(screen, lv_color_hex(0xF5F7FA), 0);

  s_title = lv_label_create(screen);
  lv_label_set_text(s_title, "CYD MQTT Dashboard");
  lv_obj_set_style_text_font(s_title, &lv_font_montserrat_20, 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 8);

  s_wifi = lv_label_create(screen);
  lv_obj_align(s_wifi, LV_ALIGN_TOP_LEFT, 10, 38);

  s_mqtt = lv_label_create(screen);
  lv_obj_align(s_mqtt, LV_ALIGN_TOP_LEFT, 110, 38);

  s_telemetry = lv_label_create(screen);
  lv_obj_align(s_telemetry, LV_ALIGN_TOP_LEFT, 210, 38);

  s_temperature = lv_label_create(screen);
  lv_obj_set_style_text_font(s_temperature, &lv_font_montserrat_14, 0);
  lv_obj_align(s_temperature, LV_ALIGN_LEFT_MID, 12, -60);

  s_humidity = lv_label_create(screen);
  lv_obj_set_style_text_font(s_humidity, &lv_font_montserrat_14, 0);
  lv_obj_align(s_humidity, LV_ALIGN_LEFT_MID, 12, -34);

  s_pressure = lv_label_create(screen);
  lv_obj_set_style_text_font(s_pressure, &lv_font_montserrat_14, 0);
  lv_obj_align(s_pressure, LV_ALIGN_LEFT_MID, 12, -8);

  s_battery = lv_label_create(screen);
  lv_obj_set_style_text_font(s_battery, &lv_font_montserrat_14, 0);
  lv_obj_align(s_battery, LV_ALIGN_LEFT_MID, 12, 18);

  s_solarVoltage = lv_label_create(screen);
  lv_obj_set_style_text_font(s_solarVoltage, &lv_font_montserrat_14, 0);
  lv_obj_align(s_solarVoltage, LV_ALIGN_LEFT_MID, 12, 44);

  s_solarCurrent = lv_label_create(screen);
  lv_obj_set_style_text_font(s_solarCurrent, &lv_font_montserrat_14, 0);
  lv_obj_align(s_solarCurrent, LV_ALIGN_LEFT_MID, 12, 70);

  s_footer = lv_label_create(screen);
  lv_obj_set_style_text_color(s_footer, lv_color_hex(0x9FB3C8), 0);
  lv_obj_align(s_footer, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void update(const AppState& state) {
  char buffer[80] = {};

  snprintf(buffer, sizeof(buffer), "WiFi %s", yesNo(state.wifiConnected));
  setLabel(s_wifi, buffer);

  snprintf(buffer, sizeof(buffer), "MQTT %s", yesNo(state.mqttConnected));
  setLabel(s_mqtt, buffer);

  snprintf(buffer,
           sizeof(buffer),
           "Data %s",
           state.latestSensor.valid ? (state.latestSensor.stale ? "STALE" : "LIVE") : "NO DATA");
  setLabel(s_telemetry, buffer);

  formatFloatValue(buffer,
                   sizeof(buffer),
                   "Outside Temp",
                   state.latestSensor.outsideTemperatureC,
                   state.latestSensor.outsideTemperatureValid,
                   "C");
  setLabel(s_temperature, buffer);

  formatFloatValue(buffer,
                   sizeof(buffer),
                   "Outside Humidity",
                   state.latestSensor.outsideHumidityPercent,
                   state.latestSensor.outsideHumidityValid,
                   "%");
  setLabel(s_humidity, buffer);

  formatFloatValue(buffer,
                   sizeof(buffer),
                   "Absolute Pressure",
                   state.latestSensor.absolutePressurehPa,
                   state.latestSensor.absolutePressureValid,
                   "hPa");
  setLabel(s_pressure, buffer);

  formatFloatValue(buffer,
                   sizeof(buffer),
                   "Battery",
                   state.latestSensor.batteryPercent,
                   state.latestSensor.batteryPercentValid,
                   "%",
                   0);
  setLabel(s_battery, buffer);

  formatFloatValue(buffer,
                   sizeof(buffer),
                   "Solar Voltage",
                   state.latestSensor.solarPanelVoltageV,
                   state.latestSensor.solarPanelVoltageValid,
                   "V");
  setLabel(s_solarVoltage, buffer);

  formatFloatValue(buffer,
                   sizeof(buffer),
                   "Solar Current",
                   state.latestSensor.solarPanelCurrentmA,
                   state.latestSensor.solarPanelCurrentValid,
                   "mA");
  setLabel(s_solarCurrent, buffer);

  const uint32_t ageMs = state.lastMqttReceiveMs == 0 ? 0 : state.uptimeMs - state.lastMqttReceiveMs;
  snprintf(buffer,
           sizeof(buffer),
           "uptime %lus  last update %lus",
           static_cast<unsigned long>(state.uptimeMs / 1000UL),
           static_cast<unsigned long>(ageMs / 1000UL));
  setLabel(s_footer, buffer);
}

}  // namespace DashboardScreen
