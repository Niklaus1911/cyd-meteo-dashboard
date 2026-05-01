# CYD Meteo Dashboard

Dashboard meteo per **CYD / Cheap Yellow Display 2 USB** basato su ESP32, Arduino, PlatformIO, LVGL e MQTT.

Il progetto mostra su display ILI9341 i dati ricevuti da un nodo ESPHome remoto, con interfaccia touch resistiva XPT2046 e pagina impostazioni per azzerare le credenziali WiFi/MQTT salvate.

## Stato del progetto

- Display 320x240 in landscape, rotazione 1.
- Tema scuro LVGL ottimizzato per CYD.
- WiFi e MQTT configurabili tramite WiFiManager.
- Topic sensori ESPHome hardcoded nel firmware.
- Supporto touch resistivo calibrato.
- Pagina Settings raggiungibile dal pulsante ingranaggio.
- Reset sicuro delle credenziali WiFi/MQTT con conferma e riavvio.

## Hardware supportato

Configurazione verificata:

- Scheda: CYD / Cheap Yellow Display, versione 2 USB.
- Display: ILI9341.
- Driver TFT: `ILI9341_2_DRIVER`.
- Rotazione: `1`.
- Inversione colori: attiva.
- Ordine colori: `TFT_BGR`.
- Backlight: GPIO `21`.
- Touch: XPT2046 resistivo.

Pin principali:

| Funzione | GPIO |
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

## Dati visualizzati

Il dashboard mostra:

- Temperatura esterna.
- Umidità esterna.
- Pressione assoluta.
- Tensione pannello solare.
- Corrente pannello solare.
- Livello batteria 18650.
- Stato WiFi/MQTT.
- Stato telemetria: `LIVE`, `STALE`, `NO DATA`.
- Età ultimo aggiornamento e uptime.

Formati numerici:

- Temperatura: `19.7 °C`.
- Umidità: `70%`.
- Batteria: `88.5%`.
- Tensione solare: `4.295 V`.
- Corrente solare: `46.903 mA`.
- Pressione: `1012.18 hPa`.

## Topic MQTT

Il firmware si aspetta un nodo ESPHome con `topic_prefix`:

```text
esp-c3-meteo-v2
```

Topic sottoscritti:

```text
esp-c3-meteo-v2/sensor/18650_battery_level/state
esp-c3-meteo-v2/sensor/outside_temperature/state
esp-c3-meteo-v2/sensor/solar_raw_voltage/state
esp-c3-meteo-v2/sensor/solar_panel_current/state
esp-c3-meteo-v2/sensor/outside_humidity/state
esp-c3-meteo-v2/sensor/absolute_pressure/state
```

I topic sono definiti in `include/AppConfig.h`.

## Logica LIVE / STALE / NO DATA

Il nodo sensore ESPHome è pensato per svegliarsi circa ogni 10 minuti.

- `LIVE`: almeno un valore valido ricevuto e ultimo aggiornamento più recente di 15 minuti.
- `STALE`: esistono valori validi, ma l'ultimo aggiornamento ha almeno 15 minuti.
- `NO DATA`: nessun valore valido ricevuto dal boot.

Il dashboard non marca i dati come obsoleti durante il normale sonno del sensore.

## Configurazione WiFi e MQTT

Al primo avvio, o dopo reset credenziali, parte il captive portal WiFiManager:

```text
CYD-Dashboard-Setup
```

Dal portale si configurano:

- rete WiFi;
- host broker MQTT;
- porta MQTT;
- utente MQTT;
- password MQTT.

La password non viene mostrata nel dashboard.

## Touch e Settings

Il touch resistivo XPT2046 è registrato come input device LVGL.

Dal dashboard:

1. Tocca l'ingranaggio in alto a destra.
2. Si apre la pagina `Settings`.
3. Da lì puoi tornare indietro o aprire la conferma `Reset WiFi/MQTT`.
4. `Erase` invia una richiesta alla task di rete, cancella le credenziali e riavvia.

La UI non cancella credenziali direttamente: invia un comando alla `NetworkTask`.

## Calibrazione touch

La calibrazione è in:

```text
include/display/TouchConfig.h
```

Valori attuali:

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

Per debug:

```cpp
DebugLogTouches = true
ShowTouchDebugOverlay = true
```

Lasciarli `false` durante l'uso normale per evitare log inutili o overlay sul dashboard.

## Architettura

Il progetto usa due task FreeRTOS separate:

- `NetworkTask`, pinned su core 0:
  - WiFi;
  - WiFiManager;
  - MQTT;
  - parsing dei messaggi MQTT;
  - reset credenziali;
  - riavvio.

- `UiTask`, pinned su core 1:
  - TFT;
  - LVGL;
  - touch;
  - dashboard e pagina impostazioni.

Regole importanti:

- Solo `UiTask` chiama API LVGL.
- La callback MQTT non chiama mai LVGL.
- La UI legge snapshot di `AppState`.
- I comandi UI verso rete passano da una queue.

## Build

Requisiti:

- PlatformIO installato.
- Toolchain ESP32 configurata da PlatformIO.

Compilazione:

```bash
/home/giuseppe/.platformio/penv/bin/platformio run
```

Upload:

```bash
/home/giuseppe/.platformio/penv/bin/platformio run --target upload
```

Monitor seriale:

```bash
/home/giuseppe/.platformio/penv/bin/platformio device monitor
```

Baud rate:

```text
115200
```

## File principali

```text
platformio.ini                    Configurazione PlatformIO e TFT_eSPI
include/AppConfig.h               Costanti applicative e topic MQTT
include/display/DisplayConfig.h   Rotazione, inversione, backlight
include/display/TouchConfig.h     Pin e calibrazione touch
src/tasks/NetworkTask.cpp         WiFi, MQTT, WiFiManager, reset
src/tasks/UiTask.cpp              Task UI e loop LVGL
src/ui/DashboardScreen.cpp        Dashboard, Settings, conferma reset
src/ui/TouchInput.cpp             Driver touch LVGL
src/ui/LvglPort.cpp               Porting LVGL/TFT
```

## Note di sicurezza

- Non salvare password o token nel codice.
- Le credenziali MQTT vengono inserite dal portale WiFiManager e salvate in NVS.
- Il repository contiene solo nomi di variabili, placeholder e topic hardcoded.
- Prima di pubblicare modifiche, controllare sempre che non ci siano credenziali reali.

## Licenza

Licenza non ancora specificata.
