#pragma once
#include <Arduino.h>

// Bellek/PSRAM/calisma suresi bilgisini g_systemStatus'a yazar; ekran
// parlakligini (Waveshare'in Set_Backlight PWM yontemiyle) ve ekran uyku
// zamanlayicisini yonetir.

void system_service_update_stats();

// Parlaklik 0-100 arasinda anlik uygulanir (Waveshare backlight PWM).
void system_service_set_brightness(uint8_t percent);
uint8_t system_service_get_brightness();

// Uyku suresi: 0 = kapali. ui_app_inactivity_ms() bu deger asildiginda
// ekran karartilir; herhangi bir dokunma LVGL'in etkinlik sayacini
// otomatik sifirladigi icin bir sonraki cagride tekrar acilir.
void system_service_set_sleep_timeout_ms(uint32_t timeout_ms);
uint32_t system_service_get_sleep_timeout_ms();
void system_service_apply_sleep_policy();
bool system_service_is_screen_sleeping();
