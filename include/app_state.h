#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Merkezi uygulama durumu.
//
// UI kodu dogrudan Wi-Fi/MQTT/SD/RTC kutuphanelerine erismez; yalnizca bu
// yapilari okur. Servisler (wifi_service, mqtt_service, storage_service,
// battery_service, rtc_service) bu yapilari gunceller.
// ---------------------------------------------------------------------------

struct SystemStatus {
  bool wifiConnected = false;
  bool mqttConnected = false;
  bool sdMounted = false;
  bool rtcValid = false;
  bool bleConnected = false; // Android uygulamasi BLE ile bagli

  char wifiSsid[33] = "";
  char wifiIp[16] = "";
  char wifiMac[18] = "";
  int wifiRssi = 0;
  uint32_t wifiReconnectCount = 0;

  char mqttHost[64] = "";
  uint16_t mqttPort = 0;
  char mqttClientId[32] = "";
  char mqttLastTopic[64] = "";
  uint32_t mqttLastMessageMs = 0;
  uint32_t mqttReconnectCount = 0;

  uint64_t sdCapacityBytes = 0;
  uint64_t sdUsedBytes = 0;

  float batteryVoltage = 0.0f;
  int batteryPercent = 0;
  bool batteryValid = false;

  uint32_t freeHeap = 0;
  uint32_t freePsram = 0;
  bool psramPresent = false;
  uint32_t uptimeSeconds = 0;

  uint8_t brightnessPercent = 50;
};

struct SensorValues {
  float temperature = 0;
  float humidity = 0;
  float voltage = 0;
  float current = 0;
  float power = 0;

  bool temperatureValid = false;
  bool humidityValid = false;
  bool voltageValid = false;
  bool currentValid = false;
  bool powerValid = false;

  uint32_t lastTemperatureUpdateMs = 0;
  uint32_t lastHumidityUpdateMs = 0;
  uint32_t lastVoltageUpdateMs = 0;
  uint32_t lastCurrentUpdateMs = 0;
  uint32_t lastPowerUpdateMs = 0;

  bool demoMode = true;
};

struct WeatherStatus {
  bool valid = false;
  float temperature = 0.0f;
  float windSpeedKmh = 0.0f;
  int weatherCode = -1;     // Open-Meteo WMO hava kodu (-1 = bilinmiyor)
  uint32_t lastUpdateMs = 0;
  uint32_t lastAttemptMs = 0;
  bool lastFetchFailed = false;
};

struct MotionStatus {
  float accelX = 0.0f;  // g
  float accelY = 0.0f;
  float accelZ = 0.0f;
  uint32_t shakeCount = 0; // her algilanan sallamada artar (UI son gordugu degerle karsilastirir)
};

#define LOG_EVENT_TEXT_LEN   40
#define LOG_EVENT_COUNT      5

struct LogEvent {
  char timeText[6];  // "HH:MM"
  char text[LOG_EVENT_TEXT_LEN];
};

struct LoggingStatus {
  bool enabled = false;
  bool lastWriteSuccessful = false;
  uint32_t rowCount = 0;
  char activeFilename[48] = "";
  char lastError[96] = "";
  uint32_t lastWriteMs = 0;
  uint32_t intervalSeconds = 60;

  LogEvent events[LOG_EVENT_COUNT];
  uint8_t eventCount = 0;
  uint32_t eventsVersion = 0; // pushEvent() her cagrildiginda artar (UI'nin listeyi yalnizca degistiginde yeniden olusturmasi icin)

  void pushEvent(const char *timeText, const char *text);
};

// "KAYDI IZLE" ekrani: SD'deki en yeni CSV dosyasinin son satirlari.
// logging_service (Driver_Loop) doldurur, UI yalnizca okur. UI, version
// degismeden listeyi yeniden olusturmaz; yeni yukleme istegini ancak mevcut
// surumu cizdikten sonra yaptigi icin iki gorev ayni anda bu yapiya dokunmaz.
#define LOG_VIEW_MAX_ROWS 30

struct LogViewRow {
  char timestamp[20];  // "YYYY-MM-DD HH:MM:SS"
  float temperature, humidity, voltage, current, power;
  bool temperatureValid, humidityValid, voltageValid, currentValid, powerValid;
};

struct LogView {
  LogViewRow rows[LOG_VIEW_MAX_ROWS]; // rows[0] = en yeni kayit
  uint8_t rowCount = 0;
  char filename[48] = "";
  char error[48] = "";
  uint32_t version = 0; // her yuklemenin sonunda artar
};

extern SystemStatus g_systemStatus;
extern SensorValues g_sensorValues;
extern LoggingStatus g_loggingStatus;
extern WeatherStatus g_weatherStatus;
extern MotionStatus g_motionStatus;
extern LogView g_logView;
