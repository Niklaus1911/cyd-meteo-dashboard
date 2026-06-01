# CYD Meteo Dashboard

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/framework-Arduino-00979D)](https://www.arduino.cc/)
[![UI](https://img.shields.io/badge/UI-LVGL%208.3-blue)](https://lvgl.io/)
[![MQTT](https://img.shields.io/badge/MQTT-PubSubClient-660066)](https://pubsubclient.knolleary.net/)
[![License](https://img.shields.io/badge/license-not%20specified-lightgrey)](#license)

ESP32 weather dashboard firmware for the **CYD / Cheap Yellow Display 2 USB**.

The project turns a CYD board into a compact MQTT dashboard for an ESPHome weather
sensor node. It shows outdoor temperature, humidity, pressure, solar panel data,
18650 battery level, connection state, telemetry freshness, and a small
Zambretti-style forecast page on the built-in ILI9341 touchscreen display.

## Features

- 320x240 LVGL dashboard optimized for the CYD landscape display.
- MQTT telemetry from an ESPHome sensor node.
- WiFi and MQTT setup through a WiFiManager captive portal.
- Touch-driven settings page with WiFi/MQTT credential reset.
- Telemetry freshness states: `LIVE`, `STALE`, and `NO DATA`.
- Persistent display rotation setting.
- Hardcoded topic defaults that can be edited in `include/AppConfig.h`.
- Separate FreeRTOS tasks for networking and UI rendering.

## Tech Stack

- Board: ESP32 CYD / Cheap Yellow Display 2 USB.
- Framework: Arduino on PlatformIO.
- UI: LVGL `8.3.x`.
- Display: TFT_eSPI with ILI9341.
- Touch: XPT2046 resistive touchscreen.
- Messaging: MQTT with PubSubClient.
- Provisioning: WiFiManager.

## Project Status

The firmware is built for one verified hardware target: the ESP32 CYD 2 USB with
ILI9341 display and XPT2046 touch. The dashboard is functional, but topic names,
touch calibration, and display settings may need adjustment for other CYD variants
or sensor setups.

## Hardware

Verified configuration:

| Part | Value |
| --- | --- |
| Board | CYD / Cheap Yellow Display 2 USB |
| Display | ILI9341 |
| TFT driver | `ILI9341_2_DRIVER` |
| Resolution | 320x240 landscape |
| Color order | `TFT_BGR` |
| Backlight | GPIO `21` |
| Touch | XPT2046 resistive |

Main pins:

| Function | GPIO |
| --- | ---: |
| TFT MOSI | 13 |
| TFT MISO | 12 |
| TFT SCLK | 14 |
| TFT CS | 15 |
| TFT DC | 2 |
| TFT BL | 21 |
| Touch CS | 33 |
| Touch IRQ | 36 |
| Touch SCLK | 25 |
| Touch MISO | 39 |
| Touch MOSI | 32 |

## Dashboard Data

The main screen displays:

- Outdoor temperature.
- Outdoor humidity.
- Absolute pressure.
- Solar panel voltage.
- Solar panel current.
- 18650 battery level.
- WiFi and MQTT connection state.
- Last update age and uptime.
- Telemetry state: `LIVE`, `STALE`, or `NO DATA`.

Expected display formats include:

| Metric | Example |
| --- | --- |
| Temperature | `19.7 C` |
| Humidity | `70%` |
| Battery | `88.5%` |
| Solar voltage | `4.295 V` |
| Solar current | `46.903 mA` |
| Pressure | `1012.18 hPa` |

## Firmware Configuration

Most project defaults are in `include/AppConfig.h`.

Edit that file when you need to change:

- MQTT client ID.
- Availability topic.
- ESPHome sensor topics.
- Forecast topics.
- MQTT buffer sizes.
- Telemetry stale timing.
- WiFiManager portal name and timeout.

MQTT broker host, port, username, and password are not hardcoded in the firmware.
They are entered through the WiFiManager setup portal and stored in ESP32 NVS.

## MQTT Topics

The firmware subscribes to ESPHome-style MQTT state topics. The default topic
constants live in `include/AppConfig.h` and should be adapted to your own
`topic_prefix`.

Expected shape:

```text
<topic_prefix>/sensor/18650_battery_level/state
<topic_prefix>/sensor/outside_temperature/state
<topic_prefix>/sensor/solar_raw_voltage/state
<topic_prefix>/sensor/solar_panel_current/state
<topic_prefix>/sensor/outside_humidity/state
<topic_prefix>/sensor/absolute_pressure/state
```

Forecast values use separate MQTT topics for region, alert, forecast text, low
summary, and update time. Those constants are also in `include/AppConfig.h`.

## LIVE, STALE, and NO DATA

The sensor node is expected to wake roughly every 10 minutes.

- `LIVE`: at least one valid value was received, and the latest update is recent.
- `STALE`: valid values exist, but the latest update is older than the configured stale threshold.
- `NO DATA`: no valid telemetry has been received since boot.

The default stale threshold is configured to tolerate normal deep-sleep intervals.

## Build and Upload

Requirements:

- PlatformIO installed.
- ESP32 PlatformIO toolchain installed.

Build:

```bash
platformio run
```

Upload:

```bash
platformio run --target upload
```

Upload to an explicit serial port:

```bash
platformio run --target upload --upload-port /dev/ttyUSB1
```

Serial monitor:

```bash
platformio device monitor
```

Monitor speed is `115200` baud.

If you use a project-local PlatformIO executable, replace `platformio` with your
local executable path.

## First Boot

On first boot, or after a credential reset, the device starts a WiFiManager
captive portal:

```text
CYD-Dashboard-Setup
```

Use the portal to configure:

- WiFi network.
- MQTT broker host.
- MQTT broker port.
- MQTT username.
- MQTT password.

The MQTT password is not shown on the dashboard.

## Touch and Settings

The XPT2046 touchscreen is registered as an LVGL input device.

From the dashboard:

1. Tap the gear button in the top-right corner.
2. Open `Settings`.
3. Use `Flip 180` to toggle display orientation.
4. Use `Reset WiFi/MQTT` to open the reset confirmation screen.
5. Confirm with `Erase` to clear saved WiFi and MQTT settings and reboot.

The UI does not clear credentials directly. It sends a command to `NetworkTask`,
which owns WiFiManager, MQTT, Preferences, and restart behavior.

## Touch Calibration

Touch calibration constants are in `include/display/TouchConfig.h`.

Current values:

```cpp
RawMinX = 289
RawMaxX = 3605
RawMinY = 562
RawMaxY = 3641
SwapXY = false
InvertX = false
InvertY = false
OffsetX = -12
OffsetY = 12
MinPressure = 200
SampleCount = 3
```

For touch debugging, temporarily enable:

```cpp
DebugLogTouches = true
ShowTouchDebugOverlay = true
```

Leave those values disabled during normal use to avoid noisy serial logs and
overlay text on the dashboard.

## Architecture

The firmware keeps networking and UI work in separate FreeRTOS tasks:

| Task | Owns |
| --- | --- |
| `NetworkTask` | WiFi, WiFiManager, MQTT, MQTT parsing, credential reset, restart |
| `UiTask` | TFT, LVGL, touch input, dashboard, settings screens |

Important rules:

- Only `UiTask` calls LVGL APIs.
- The MQTT callback never calls LVGL.
- UI code reads snapshots from `AppState`.
- UI commands for networking actions go through a FreeRTOS queue.

## Project Layout

```text
platformio.ini                    PlatformIO and TFT_eSPI configuration
include/AppConfig.h               App constants and MQTT topic defaults
include/display/DisplayConfig.h   Rotation, inversion, and backlight settings
include/display/TouchConfig.h     Touch pins and calibration values
src/tasks/NetworkTask.cpp         WiFi, MQTT, WiFiManager, reset, telemetry parsing
src/tasks/UiTask.cpp              UI task and LVGL loop
src/ui/DashboardScreen.cpp        Dashboard, settings, and reset confirmation UI
src/ui/TouchInput.cpp             LVGL touch driver
src/ui/LvglPort.cpp               LVGL and TFT porting
```

## Troubleshooting

- **Display colors are swapped:** change the TFT color order build flag between
  `TFT_BGR` and `TFT_RGB` in `platformio.ini`.
- **Touch is offset or inverted:** adjust constants in
  `include/display/TouchConfig.h`.
- **No telemetry appears:** confirm the MQTT broker settings in the setup portal
  and check that the topic constants match your ESPHome `topic_prefix`.
- **Dashboard stays stale:** verify the sensor node is publishing at the expected
  interval and adjust `TelemetryStaleAfterMs` if needed.
- **Credential reset does not start:** use the Settings page reset flow and watch
  the serial monitor for WiFiManager or MQTT reset messages.

## Security Notes

- Do not commit real WiFi passwords, MQTT passwords, tokens, or private keys.
- WiFi and MQTT credentials are entered through WiFiManager and stored in NVS.
- The dashboard can show MQTT broker host information, but it does not display
  the MQTT password.
- Review `include/AppConfig.h` before publishing changes if your topic names are
  sensitive.

## License

License not specified yet.
