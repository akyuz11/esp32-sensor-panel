#include "wifi_service.h"
#include "app_state.h"
#include "app_config.h"
#include "credentials.h"
#include "rtc_service.h"

#include <WiFi.h>

enum WifiState { WIFI_STATE_DISCONNECTED, WIFI_STATE_CONNECTING, WIFI_STATE_CONNECTED };

static WifiState s_state = WIFI_STATE_DISCONNECTED;
static uint32_t s_next_attempt_ms = 0;
static uint32_t s_connect_started_ms = 0;
static uint32_t s_backoff_ms = APP_WIFI_RETRY_MIN_MS;
static bool s_credentials_present = false;

static bool s_scan_requested = false;
static bool s_scan_running = false;
static int s_scan_result_count = -1;

#define WIFI_CONNECT_TIMEOUT_MS 10000

static void begin_connect_attempt()
{
  Serial.printf("[WIFI] Baglaniliyor: %s\r\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  s_state = WIFI_STATE_CONNECTING;
  s_connect_started_ms = millis();
}

static void schedule_retry()
{
  s_state = WIFI_STATE_DISCONNECTED;
  s_next_attempt_ms = millis() + s_backoff_ms;
  s_backoff_ms = min<uint32_t>(s_backoff_ms * 2, APP_WIFI_RETRY_MAX_MS);
}

void wifi_service_init()
{
  s_credentials_present = (strlen(WIFI_SSID) > 0);
  if (!s_credentials_present) {
    Serial.println("[WIFI] secrets.h icinde SSID tanimli degil, Wi-Fi devre disi (demo modunda calisiliyor)");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false); // Kendi geri cekilme mantigimizi kullaniyoruz.
  s_backoff_ms = APP_WIFI_RETRY_MIN_MS;
  begin_connect_attempt();
}

void wifi_service_loop()
{
  if (!s_credentials_present) {
    return;
  }

  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    if (s_state != WIFI_STATE_CONNECTED) {
      s_state = WIFI_STATE_CONNECTED;
      s_backoff_ms = APP_WIFI_RETRY_MIN_MS;
      g_systemStatus.wifiConnected = true;
      strncpy(g_systemStatus.wifiSsid, WiFi.SSID().c_str(), sizeof(g_systemStatus.wifiSsid) - 1);
      strncpy(g_systemStatus.wifiIp, WiFi.localIP().toString().c_str(), sizeof(g_systemStatus.wifiIp) - 1);
      strncpy(g_systemStatus.wifiMac, WiFi.macAddress().c_str(), sizeof(g_systemStatus.wifiMac) - 1);
      Serial.printf("[WIFI] Baglandi, IP=%s RSSI=%d dBm\r\n", g_systemStatus.wifiIp, WiFi.RSSI());

      rtc_service_sync_from_ntp();
    }
    g_systemStatus.wifiRssi = WiFi.RSSI();
  } else {
    if (s_state == WIFI_STATE_CONNECTED) {
      Serial.println("[WIFI] Baglanti koptu");
      g_systemStatus.wifiConnected = false;
      g_systemStatus.wifiReconnectCount++;
      schedule_retry();
    } else if (s_state == WIFI_STATE_CONNECTING) {
      if (millis() - s_connect_started_ms > WIFI_CONNECT_TIMEOUT_MS) {
        Serial.println("[WIFI] Baglanti zaman asimina ugradi");
        g_systemStatus.wifiReconnectCount++;
        schedule_retry();
      }
    } else if (s_state == WIFI_STATE_DISCONNECTED) {
      if (millis() >= s_next_attempt_ms) {
        begin_connect_attempt();
      }
    }
  }

  // Engellemeyen ag taramasi
  if (s_scan_requested && !s_scan_running) {
    WiFi.scanNetworks(true);
    s_scan_running = true;
    s_scan_requested = false;
    s_scan_result_count = -1;
  }
  if (s_scan_running) {
    int n = WiFi.scanComplete();
    if (n >= 0) {
      s_scan_result_count = n;
      s_scan_running = false;
      Serial.printf("[WIFI] Tarama tamamlandi: %d ag bulundu\r\n", n);
    } else if (n == WIFI_SCAN_FAILED) {
      s_scan_result_count = 0;
      s_scan_running = false;
    }
  }
}

void wifi_service_request_reconnect()
{
  if (!s_credentials_present) {
    return;
  }
  WiFi.disconnect();
  s_backoff_ms = APP_WIFI_RETRY_MIN_MS;
  s_next_attempt_ms = millis();
  s_state = WIFI_STATE_DISCONNECTED;
}

void wifi_service_request_scan()
{
  s_scan_requested = true;
}

int wifi_service_scan_result_count()
{
  return s_scan_result_count;
}
