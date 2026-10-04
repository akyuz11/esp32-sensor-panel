#pragma once

// ---------------------------------------------------------------------------
// Genel uygulama yapilandirmasi (derleme zamaninda sabit degerler)
// ---------------------------------------------------------------------------

// Saat dilimi: Turkiye icin varsayilan UTC+3
#define APP_UTC_OFFSET_HOURS   3

// Demo modu: MQTT verisi gelmeden ekrani test edebilmek icin
#define APP_DEMO_MODE_DEFAULT  true

// MQTT topic'leri (sade sayisal payload, ornek: "24.6")
#define MQTT_TOPIC_TEMPERATURE   "MQTT_Verileri/sensor/temperature"
#define MQTT_TOPIC_HUMIDITY      "MQTT_Verileri/sensor/humidity"
#define MQTT_TOPIC_VOLTAGE       "MQTT_Verileri/energy/voltage"
#define MQTT_TOPIC_CURRENT       "MQTT_Verileri/energy/current"
#define MQTT_TOPIC_POWER         "MQTT_Verileri/energy/power"

#define MQTT_CLIENT_ID_PREFIX    "esp32panel-"

// TF karta kayit araligi (saniye)
#define APP_LOG_INTERVAL_SECONDS  60

// Veri gecerlilik esikleri (saniye)
#define APP_DATA_FRESH_SECONDS    15
#define APP_DATA_STALE_SECONDS    60

// Wi-Fi yeniden baglanma geri cekilme (ms)
#define APP_WIFI_RETRY_MIN_MS     2000
#define APP_WIFI_RETRY_MAX_MS     30000

// MQTT yeniden baglanma geri cekilme (ms)
#define APP_MQTT_RETRY_MIN_MS     2000
#define APP_MQTT_RETRY_MAX_MS     30000

// Ekran uyku sureleri (ms) - 0 = kapali
#define APP_SLEEP_TIMEOUT_OFF     0
#define APP_SLEEP_TIMEOUT_30S     30000UL
#define APP_SLEEP_TIMEOUT_60S     60000UL
#define APP_SLEEP_TIMEOUT_5MIN    300000UL

// Hava durumu (Open-Meteo, API anahtari gerektirmez) - konum ve yenileme araligi
#define APP_WEATHER_CITY_NAME       "Istanbul"
#define APP_WEATHER_LATITUDE        41.0082f
#define APP_WEATHER_LONGITUDE       28.9784f
#define APP_WEATHER_UPDATE_INTERVAL_MS  (15UL * 60UL * 1000UL)  // 15 dakika

// Sallama algilama (motion_service): ~10 Hz'de ardisik iki ivme okumasi
// arasindaki vektorel fark APP_SHAKE_DELTA_G'yi APP_SHAKE_WINDOW_MS icinde
// APP_SHAKE_HITS kez asarsa bir "sallama" sayilir.
#define APP_SHAKE_DELTA_G       0.9f
#define APP_SHAKE_HITS          2
#define APP_SHAKE_WINDOW_MS     800
#define APP_SHAKE_COOLDOWN_MS   1500

// "Salla Kazan": rastgele sayilarin donme suresi ve aralik.
#define APP_LUCKY_ROLL_MS       5000
#define APP_LUCKY_MIN           0
#define APP_LUCKY_MAX           1000
