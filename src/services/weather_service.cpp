#include "weather_service.h"
#include "app_state.h"
#include "app_config.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi baglanti kurulduktan hemen sonra ilk cekimi yapabilmek icin 0'dan
// baslar (weather_service_loop ilk cagrisinda hemen deneyecek).
static uint32_t s_next_fetch_ms = 0;
static volatile bool s_refresh_requested = false;

const char *weather_service_code_text(int code)
{
  if (code == 0) return "Acik";
  if (code == 1 || code == 2) return "Az bulutlu";
  if (code == 3) return "Bulutlu";
  if (code == 45 || code == 48) return "Sisli";
  if (code >= 51 && code <= 57) return "Ciseleme";
  if (code >= 61 && code <= 67) return "Yagmurlu";
  if (code >= 71 && code <= 77) return "Karli";
  if (code >= 80 && code <= 82) return "Saganak yagmur";
  if (code >= 85 && code <= 86) return "Kar saganagi";
  if (code >= 95) return "Firtinali";
  return "Bilinmiyor";
}

static void do_fetch()
{
  g_weatherStatus.lastAttemptMs = millis();

  WiFiClientSecure client;
  client.setInsecure(); // Hava durumu icin sertifika dogrulamasi atlaniyor (kritik olmayan veri)

  HTTPClient http;
  char url[192];
  snprintf(url, sizeof(url),
      "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current_weather=true&timezone=auto",
      (double)APP_WEATHER_LATITUDE, (double)APP_WEATHER_LONGITUDE);

  if (!http.begin(client, url)) {
    Serial.println("[WEATHER] http.begin basarisiz");
    g_weatherStatus.lastFetchFailed = true;
    return;
  }
  http.setTimeout(8000);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    JsonObject cw = doc["current_weather"];
    if (!err && !cw.isNull()) {
      g_weatherStatus.temperature = cw["temperature"] | 0.0f;
      g_weatherStatus.windSpeedKmh = cw["windspeed"] | 0.0f;
      g_weatherStatus.weatherCode = cw["weathercode"] | -1;
      g_weatherStatus.valid = true;
      g_weatherStatus.lastFetchFailed = false;
      g_weatherStatus.lastUpdateMs = millis();
      Serial.printf("[WEATHER] Guncellendi: %.1f C, kod=%d (%s)\r\n",
          g_weatherStatus.temperature, g_weatherStatus.weatherCode,
          weather_service_code_text(g_weatherStatus.weatherCode));
    } else {
      Serial.println("[WEATHER] JSON ayristirma hatasi");
      g_weatherStatus.lastFetchFailed = true;
    }
  } else {
    Serial.printf("[WEATHER] HTTP hata: %d\r\n", httpCode);
    g_weatherStatus.lastFetchFailed = true;
  }
  http.end();
}

void weather_service_init()
{
  s_next_fetch_ms = 0;
  s_refresh_requested = false;
}

void weather_service_loop()
{
  uint32_t now = millis();
  bool due = (now >= s_next_fetch_ms);

  if (!due && !s_refresh_requested) {
    return;
  }
  s_refresh_requested = false;

  if (!g_systemStatus.wifiConnected) {
    // Wi-Fi yokken sik sik denemeye gerek yok; kisa bir sure sonra tekrar bak.
    s_next_fetch_ms = now + 5000;
    return;
  }

  do_fetch();
  s_next_fetch_ms = now + APP_WEATHER_UPDATE_INTERVAL_MS;
}

void weather_service_request_refresh()
{
  s_refresh_requested = true;
}
