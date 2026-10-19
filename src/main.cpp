#include <Arduino.h>
#include <lvgl.h>

#include "app_config.h"

#include "board.h"

#include "ui/ui_app.h"
#include "services/demo_service.h"
#include "services/battery_service.h"
#include "services/rtc_service.h"
#include "services/storage_service.h"
#include "services/wifi_service.h"
#include "services/system_service.h"
#include "services/mqtt_service.h"
#include "services/logging_service.h"
#include "services/weather_service.h"
#include "services/motion_service.h"
#include "services/web_service.h"
#include "services/ble_service.h"

// ---------------------------------------------------------------------------
// Donanim board.h uzerinden baslatilir (board_init + SD + board_display_init;
// kartin kendi baslatma sirasi src/board/<kart>/board_impl.cpp icindedir),
// ardindan dort sayfali uygulama arayuzu (ui_app) ve servisler
// (pil/RTC/SD/Wi-Fi/sistem) kurulur.
// ---------------------------------------------------------------------------

static void Driver_Loop(void *parameter)
{
  uint32_t last_battery_ms = 0;
  uint32_t last_rtc_ms = 0;
  uint32_t last_storage_ms = 0;
  uint32_t last_system_stats_ms = 0;

  while (1) {
    board_poll();
    motion_service_loop();
    ble_service_loop();
    demo_service_loop();
    wifi_service_loop();
    rtc_service_process_requests();
    mqtt_service_loop();
    logging_service_loop();
    weather_service_loop();

    uint32_t now = millis();

    if (now - last_battery_ms >= 5000) {
      last_battery_ms = now;
      battery_service_update();
    }
    if (now - last_rtc_ms >= 1000) {
      last_rtc_ms = now;
      rtc_service_update();
    }
    if (now - last_storage_ms >= 5000) {
      last_storage_ms = now;
      storage_service_update();
    }
    if (now - last_system_stats_ms >= 5000) {
      last_system_stats_ms = now;
      system_service_update_stats();
    }

    system_service_apply_sleep_policy();

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(200);
  Serial.println("[BOOT] Application starting");

  Serial.printf("[BOOT] Kart: %s\r\n", board_name());
  board_init();
  Serial.println("[BOOT] board_init tamamlandi");

  storage_service_init();
  storage_service_run_boot_test();

  board_display_init();
  Serial.println("[UI] Ekran ve LVGL baslatildi");

  ui_app_init();
  Serial.println("[UI] Dort sayfali arayuz olusturuldu");

  system_service_set_brightness(50);

  wifi_service_init();
  mqtt_service_init();
  logging_service_init();
  weather_service_init();
  motion_service_init();
  web_service_init();
  ble_service_init();

  xTaskCreatePinnedToCore(
      Driver_Loop,
      "DriverLoop",
      8192,
      NULL,
      3,
      NULL,
      0);

  Serial.println("[BOOT] Kurulum tamamlandi");
}

void loop()
{
  board_ui_loop();
  vTaskDelay(pdMS_TO_TICKS(5));
}
