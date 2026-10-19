# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Firmware for the **Waveshare ESP32-S3-Touch-LCD-1.46** (ESP32-S3R8, 16MB Flash, 8MB Octal PSRAM, 412×412 round SPD2010 touch LCD). PlatformIO + Arduino framework + LVGL 8. It's a four-page IoT sensor monitoring/logging panel: DURUM (status/battery/clock), VERILER (live MQTT sensor cards), MENU (settings sub-screens), KAYIT (SD card CSV logging).

## Commands

```bash
pio run                 # build
pio run -t upload       # flash to board
pio run -t clean        # clean build artifacts
pio device monitor      # serial monitor (115200 baud)
pio run -t upload -t monitor   # flash + immediately open monitor
```

There is no test suite. `platformio.ini` has two environments: `waveshare_esp32s3_touch_lcd_146` (the default, so plain `pio run` targets it) and `crowpanel_advance_28` (use `-e crowpanel_advance_28`). Both boards enumerate with the same USB VID:PID, so `tools/select_upload_port.py` picks the upload port by each env's `custom_usb_serial` and aborts the upload if that board isn't plugged in — never remove it or hard-code a COM port, or firmware can land on the wrong board.

Before building, `include/secrets.h` must exist (copy from `include/secrets.example.h`). If missing, the build still succeeds via `include/credentials.h`'s `__has_include` fallback, but Wi-Fi/MQTT stay disabled and the app runs in DEMO mode only.

## Critical platform quirk — read before touching `platformio.ini` or `boards/`

This board's official Waveshare demo requires **Arduino-ESP32 core 3.1.1 / ESP-IDF 5.3.2** (for the QSPI-based SPD2010 panel driver). The stock PlatformIO `espressif32` platform does not ship that core version. `platformio.ini` therefore pins `platform` to a specific **pioarduino** fork release (`53.03.11`) that matches exactly. Do not "upgrade" this to `platform = espressif32` or to a newer pioarduino tag without verifying the bundled Arduino core version still matches what the vendor driver code expects (see `example/` reference in project history — the driver uses `esp_lcd_panel_io_spi_config_t.flags.quad_mode`, which requires the newer IDF).

Similarly, `boards/waveshare_esp32s3_touch_lcd_146.json` is a **custom local board definition** (no upstream board JSON exists for this exact N16R8 flash/PSRAM combo in the pinned platform release). It sets `memory_type: qio_opi` and `-DBOARD_HAS_PSRAM`. Don't delete it or swap to a generic `esp32-s3-devkitc-1` board — that board defaults to no-PSRAM and a different partition table.

`include/lv_conf.h` is discovered by LVGL via `-DLV_CONF_INCLUDE_SIMPLE=1 -Iinclude` in `build_flags`. LVGL is pinned to `8.3.10` (matches the vendor demo); do not bump to LVGL 9 — the whole UI layer uses LVGL 8 APIs (`lv_obj_t *`/`lv_disp_drv_t` style, not the 9.x object model).

## Architecture

### Board abstraction (`include/board.h`, `src/board/<board>/`)

`main.cpp` and every service reach hardware **only** through `include/board.h` (init/poll, backlight, battery volts, RTC read/write/lost-power, IMU accel/gyro, SD fs). Nothing outside `src/board/` may include a vendor header. Each board implements `board.h` in `src/board/<board>/board_impl.cpp`; `build_src_filter` in each `platformio.ini` env compiles only that board's folder (`-<board/> +<board/waveshare_146/>`). Hardware a board lacks is reported via `board_has_*()` and "empty" return values rather than `#ifdef`s in services. When adding a new hardware access, extend `board.h` and every `board_impl.cpp` — don't include vendor headers from a service.

### Vendor driver boundary (`src/board/waveshare_146/vendor/`)

These files are copied **verbatim** from Waveshare's official Arduino demo (LCD/touch/TCA9554/battery/RTC/SD/backlight/IMU/power-key drivers) and should be treated as a vendored dependency, not application code — avoid editing them. If a bug is suspected here, prefer working around it from `board_impl.cpp` rather than patching the driver. Pin assignments live in these headers (`I2C_Driver.h`, `Display_SPD2010.h`, `Touch_SPD2010.h`, `BAT_Driver.h`, `SD_Card.h`, `PWR_Key.h`) — if you need a pin number, look here rather than guessing.

`TCA9554PWR` is an I2C GPIO expander; some "pins" referenced in driver code (e.g. LCD reset, SD D3 enable) are `EXIO_PIN*` virtual pins on that expander, not real ESP32 GPIOs.

### Two-task concurrency model — the load-bearing rule of this codebase

- **Arduino `loop()`** (implicit core, effectively the main/UI task): calls only `Lvgl_Loop()` + `vTaskDelay(5)`. This is the only place `lv_timer_handler()` runs, so **anything that touches LVGL objects must happen on this task** — either directly in `ui_app_init()`/page create functions, or inside the periodic `lv_timer` in `ui_app.cpp` (`refresh_timer_cb`, 1s).
- **`Driver_Loop` FreeRTOS task** (pinned to core 0, created in `main.cpp::setup()`): runs every ~100ms and drives *all* blocking/IO work — board polling via `board_poll()` (on Waveshare: `PWR_Loop`, `QMI8658_Loop`, `PCF85063_Loop`, `BAT_Get_Volts`), and every service's `_loop()`/`_update()` (wifi, mqtt, rtc, storage, system, logging, demo).

**Consequence:** LVGL button/slider callbacks in `src/ui/ui_pages.cpp` never call blocking service functions directly (no `WiFi.begin()`, no NTP sync, no SD writes, no MQTT connect from inside an `lv_event_cb_t`). Instead they set a `volatile bool` request flag (e.g. `wifi_service_request_reconnect()`, `rtc_service_request_ntp_sync()`, `logging_service_request_write_now()`, `mqtt_service_request_connect()`), and `Driver_Loop` polls/executes it on the next iteration. When adding a new UI action that touches network/SD/RTC-write, follow this same request-flag pattern — don't be the first one to block the UI thread.

### Central state (`include/app_state.h` / `src/app_state.cpp`)

Three global structs — `g_systemStatus`, `g_sensorValues`, `g_loggingStatus` — are the *only* channel between services and UI. Services (in `src/services/`) write into these; `src/ui/ui_pages.cpp` only reads from them (plus calls the `*_request_*()` action functions). Don't wire a service directly to a UI widget pointer — go through the struct.

`LoggingStatus::pushEvent()` bumps `eventsVersion`; `ui_page_log_update()` compares that counter before rebuilding the on-screen event list, so it doesn't recreate LVGL objects every refresh tick. Follow this compare-before-rebuild pattern for any other "list-like" UI that updates on a timer — the project rule is real widgets get `lv_label_set_text`/`lv_arc_set_value` style updates every tick, and `lv_obj_del`/recreate only when the underlying data actually changed.

### Services (`src/services/`)

Each service owns one concern and exposes an `_init()` + `_loop()`/`_update()` pair called from `main.cpp`, plus `_request_*()` functions safe to call from LVGL callbacks:

- `wifi_service` — non-blocking connect state machine with exponential backoff (`APP_WIFI_RETRY_MIN_MS`..`APP_WIFI_RETRY_MAX_MS` in `app_config.h`); triggers `rtc_service_sync_from_ntp()` on connect.
- `mqtt_service` — PubSubClient wrapper, subscribes to the 5 topics in `app_config.h`, parses plain numeric payloads (`strtof`, rejects NaN/inf, keeps last-good value on bad payload). Sets `g_sensorValues.demoMode = false` the moment any real payload lands (see `demo_service` below).
- `demo_service` — only runs while `g_sensorValues.demoMode == true`; generates slowly-oscillating fake sensor values so page 2 is testable without a broker.
- `rtc_service` — reads the vendor's `datetime` global (populated by `PCF85063_Loop()`), independently re-reads the PCF85063 seconds register to check the oscillator-stop (OS) flag for validity (the vendor's `PCF85063_Read_Time()` masks that bit off, so this is a deliberate second read, not a vendor-file edit).
- `battery_service` — interprets `BAT_analogVolts` (vendor-measured) via a hardcoded Li-Po voltage→percent table; there is no physical "battery present" pin, so presence/validity is inferred from voltage thresholds (documented in-file as approximate).
- `storage_service` — SD mount/capacity tracking + periodic remount attempts if the card was pulled.
- `logging_service` — the 60s CSV writer (`/logs/YYYY-MM-DD.csv`); refuses to write if SD isn't mounted or `g_systemStatus.rtcValid` is false (never fabricates a timestamp/filename).
- `ble_service` — BLE GATT server ("Deney-Karti") for the Android app in `android-app/`: notifies a 20-byte motion packet (v1) and a 20-byte sensor/status packet (v2), and accepts phone commands (set RTC time, log now, brightness, logging on/off). Command writes arrive on the BLE stack's task, so they are copied into a buffer and executed in `ble_service_loop()` on `Driver_Loop`. Packet layouts must match `Packet.java` / `SensorPacket.java` byte for byte. Build the app with the local JDK/Gradle/SDK in `tools/android-build/` (`gradle assembleDebug` in `android-app/`).
- `system_service` — heap/PSRAM/uptime stats, backlight brightness (wraps vendor `Set_Backlight`), and the inactivity-based screen-sleep policy (uses LVGL's own `lv_disp_get_inactive_time`, exposed via `ui_app_inactivity_ms()`).

### UI (`src/ui/`)

- `ui_app.cpp` — builds the `lv_tileview` (4 horizontal tiles) + fixed bottom nav bar, syncs nav-button highlight with tileview swipe via `LV_EVENT_VALUE_CHANGED`, owns the 1s refresh timer.
- `ui_pages.cpp` — all four pages' create/update functions live here in one file (status/data/menu/log), plus every button/slider callback and sub-screen content builder.
- `ui_modal.cpp` — generic full-screen modal helper (`ui_modal_open(title)` → content container, auto GERI button) used for every detail popup and MENU sub-screen. Only one modal exists at a time; opening a new one closes the old one.
- `ui_theme.h` — the round-screen safe-area constants (`UI_SAFE_*`, `UI_CONTENT_*`, `UI_NAV_HEIGHT`) and dark palette. Any new page content must stay inside `UI_CONTENT_*` bounds, not the full 412×412 square.

### Configuration layering

`app_config.h` — compile-time constants (MQTT topics, intervals, thresholds, UTC offset). `include/secrets.h` (gitignored) — Wi-Fi/MQTT credentials, templated by `secrets.example.h`. `include/credentials.h` — the `__has_include("secrets.h")` shim that makes secrets optional at build time.
