#include "rtc_service.h"
#include "app_state.h"
#include "app_config.h"
#include "board.h"

#include <time.h>
#include <stdio.h>

static char s_time_text[6] = "";
static char s_date_text[11] = "";
static uint32_t s_last_ntp_sync_ms = 0;
static bool s_ntp_ever_synced = false;

void rtc_service_update()
{
  // OS (osilator durdu) bayragi: guc kesintisi sonrasi zaman guvenilmez.
  bool os_stopped = board_rtc_lost_power();
  BoardDateTime now;
  board_rtc_now(&now);
  bool year_sane = (now.year >= 2024 && now.year <= 2099);

  g_systemStatus.rtcValid = !os_stopped && year_sane;

  if (!g_systemStatus.rtcValid) {
    s_time_text[0] = '\0';
    s_date_text[0] = '\0';
    return;
  }

  snprintf(s_time_text, sizeof(s_time_text), "%02u:%02u", now.hour, now.minute);
  snprintf(s_date_text, sizeof(s_date_text), "%02u.%02u.%04u", now.day, now.month, now.year);
}

const char *rtc_service_time_text() { return s_time_text; }
const char *rtc_service_date_text() { return s_date_text; }

bool rtc_service_sync_from_ntp()
{
  configTime(APP_UTC_OFFSET_HOURS * 3600, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 5000)) {
    Serial.println("[RTC] NTP senkronizasyonu basarisiz, mevcut RTC zamani korunuyor");
    return false;
  }

  BoardDateTime dt;
  dt.year = timeinfo.tm_year + 1900;
  dt.month = timeinfo.tm_mon + 1;
  dt.day = timeinfo.tm_mday;
  dt.dotw = timeinfo.tm_wday;
  dt.hour = timeinfo.tm_hour;
  dt.minute = timeinfo.tm_min;
  dt.second = timeinfo.tm_sec;

  board_rtc_write(dt);
  s_last_ntp_sync_ms = millis();
  s_ntp_ever_synced = true;
  Serial.println("[RTC] NTP ile senkronize edildi ve RTC'ye yazildi");
  return true;
}

bool rtc_service_write_system_time_to_rtc()
{
  if (!s_ntp_ever_synced) {
    Serial.println("[RTC] Once NTP ile esitlenmeli, sistem saati bilinmiyor");
    return false;
  }

  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  BoardDateTime dt;
  dt.year = timeinfo.tm_year + 1900;
  dt.month = timeinfo.tm_mon + 1;
  dt.day = timeinfo.tm_mday;
  dt.dotw = timeinfo.tm_wday;
  dt.hour = timeinfo.tm_hour;
  dt.minute = timeinfo.tm_min;
  dt.second = timeinfo.tm_sec;

  board_rtc_write(dt);
  Serial.println("[RTC] Sistem saati RTC'ye yazildi");
  return true;
}

bool rtc_service_set_from_phone(uint16_t year, uint8_t month, uint8_t day,
                                uint8_t hour, uint8_t minute, uint8_t second,
                                uint8_t dotw)
{
  if (year < 2024 || year > 2099 || month < 1 || month > 12 || day < 1 || day > 31 ||
      hour > 23 || minute > 59 || second > 59 || dotw > 6) {
    Serial.println("[RTC] Telefondan gelen saat gecersiz, yok sayildi");
    return false;
  }

  BoardDateTime dt;
  dt.year = year;
  dt.month = month;
  dt.day = day;
  dt.dotw = dotw;
  dt.hour = hour;
  dt.minute = minute;
  dt.second = second;

  // Saniye kaydinin yazilmasi OS bayragini da temizler.
  board_rtc_write(dt);
  Serial.println("[RTC] Saat telefondan ayarlandi");
  return true;
}

uint32_t rtc_service_last_ntp_sync_ms() { return s_last_ntp_sync_ms; }
bool rtc_service_ntp_ever_synced() { return s_ntp_ever_synced; }

static volatile bool s_ntp_sync_requested = false;
static volatile bool s_write_to_rtc_requested = false;

void rtc_service_request_ntp_sync() { s_ntp_sync_requested = true; }
void rtc_service_request_write_to_rtc() { s_write_to_rtc_requested = true; }

void rtc_service_process_requests()
{
  if (s_ntp_sync_requested) {
    s_ntp_sync_requested = false;
    rtc_service_sync_from_ntp();
  }
  if (s_write_to_rtc_requested) {
    s_write_to_rtc_requested = false;
    rtc_service_write_system_time_to_rtc();
  }
}
