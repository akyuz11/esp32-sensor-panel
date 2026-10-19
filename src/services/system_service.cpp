#include "system_service.h"
#include "app_state.h"
#include "app_config.h"
#include "board.h"
#include "ui/ui_app.h"

#include <esp_heap_caps.h>

static uint8_t s_user_brightness = 50;
static uint32_t s_sleep_timeout_ms = APP_SLEEP_TIMEOUT_OFF;
static bool s_screen_sleeping = false;

void system_service_update_stats()
{
  g_systemStatus.freeHeap = ESP.getFreeHeap();
  g_systemStatus.psramPresent = psramFound();
  g_systemStatus.freePsram = g_systemStatus.psramPresent ? ESP.getFreePsram() : 0;
  g_systemStatus.uptimeSeconds = millis() / 1000;
}

void system_service_set_brightness(uint8_t percent)
{
  if (percent > 100) percent = 100;
  s_user_brightness = percent;
  g_systemStatus.brightnessPercent = percent;
  if (!s_screen_sleeping) {
    board_set_backlight(percent);
  }
}

uint8_t system_service_get_brightness()
{
  return s_user_brightness;
}

void system_service_set_sleep_timeout_ms(uint32_t timeout_ms)
{
  s_sleep_timeout_ms = timeout_ms;
}

uint32_t system_service_get_sleep_timeout_ms()
{
  return s_sleep_timeout_ms;
}

void system_service_apply_sleep_policy()
{
  if (s_sleep_timeout_ms == APP_SLEEP_TIMEOUT_OFF) {
    if (s_screen_sleeping) {
      s_screen_sleeping = false;
      board_set_backlight(s_user_brightness);
    }
    return;
  }

  uint32_t inactive_ms = ui_app_inactivity_ms();

  if (!s_screen_sleeping && inactive_ms >= s_sleep_timeout_ms) {
    s_screen_sleeping = true;
    board_set_backlight(0);
    Serial.println("[UI] Ekran uykuya gecti");
  } else if (s_screen_sleeping && inactive_ms < s_sleep_timeout_ms) {
    s_screen_sleeping = false;
    board_set_backlight(s_user_brightness);
    Serial.println("[UI] Ekran uyandi");
  }
}

bool system_service_is_screen_sleeping()
{
  return s_screen_sleeping;
}
