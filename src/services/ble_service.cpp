#include "ble_service.h"
#include "app_state.h"
#include "rtc_service.h"
#include "logging_service.h"
#include "system_service.h"
#include "board/waveshare/Gyro_QMI8658.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <atomic>
#include <math.h>
#include <string.h>

#define BLE_SERVICE_UUID  "87ba0001-6d31-4b6e-8bd0-ef38247a9100"
#define BLE_MOTION_UUID   "87ba0002-6d31-4b6e-8bd0-ef38247a9100"
#define BLE_SENSORS_UUID  "87ba0003-6d31-4b6e-8bd0-ef38247a9100"
#define BLE_COMMAND_UUID  "87ba0004-6d31-4b6e-8bd0-ef38247a9100"

// Komut kodlari (Android: MainActivity.CMD_*)
#define CMD_SET_TIME     0x01 // yil LE16, ay, gun, saat, dk, sn, haftagunu(0=Pazar)
#define CMD_LOG_NOW      0x02
#define CMD_BRIGHTNESS   0x03 // 0-100
#define CMD_LOGGING      0x04 // 0 = durdur, 1 = baslat

static BLECharacteristic *s_data = nullptr;
static BLECharacteristic *s_sensors = nullptr;
static std::atomic<bool> s_connected{false};
static std::atomic<bool> s_restart{false};
static uint32_t s_last = 0;
static uint32_t s_last_sensors = 0;

// Yazma geri cagrisi BLE yigininin gorevinde calisir; I2C/SD/PWM islemleri
// orada yapilmaz. Komut buraya kopyalanir ve Driver_Loop'ta uygulanir.
static portMUX_TYPE s_cmd_mux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t s_cmd[10];
static size_t s_cmd_len = 0;
static bool s_cmd_pending = false;

class Callbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    s_connected.store(true);
    Serial.println("[BLE] Telefon baglandi");
  }
  void onDisconnect(BLEServer *) override {
    s_connected.store(false);
    s_restart.store(true);
    Serial.println("[BLE] Telefon ayrildi");
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    size_t len = c->getLength();
    if (len == 0 || len > sizeof(s_cmd)) return;
    portENTER_CRITICAL(&s_cmd_mux);
    memcpy(s_cmd, c->getData(), len);
    s_cmd_len = len;
    s_cmd_pending = true;
    portEXIT_CRITICAL(&s_cmd_mux);
  }
};

void ble_service_init() {
  BLEDevice::init("Deney-Karti");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new Callbacks());
  BLEService *service = server->createService(BLE_SERVICE_UUID);
  s_data = service->createCharacteristic(BLE_MOTION_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  s_data->addDescriptor(new BLE2902());
  s_sensors = service->createCharacteristic(BLE_SENSORS_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  s_sensors->addDescriptor(new BLE2902());
  BLECharacteristic *command = service->createCharacteristic(BLE_COMMAND_UUID,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  command->setCallbacks(new CommandCallbacks());
  service->start();
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(service->getUUID());
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("[BLE] Deney-Karti hazir");
}

static void put16(uint8_t *p, int value) {
  p[0] = value & 255;
  p[1] = (value >> 8) & 255;
}

static void put32(uint8_t *p, int32_t value) {
  for (int i = 0; i < 4; ++i) p[i] = ((uint32_t)value >> (8*i)) & 255;
}

static int scaled(float v, float scale) {
  if (!isfinite(v)) return 0;
  return (int)lroundf(fmaxf(-32768, fminf(32767, v * scale)));
}

static int scaledU16(float v, float scale) {
  if (!isfinite(v)) return 0;
  return (int)lroundf(fmaxf(0, fminf(65535, v * scale)));
}

static void process_command() {
  uint8_t cmd[sizeof(s_cmd)];
  size_t len;
  portENTER_CRITICAL(&s_cmd_mux);
  bool pending = s_cmd_pending;
  s_cmd_pending = false;
  len = s_cmd_len;
  memcpy(cmd, s_cmd, len);
  portEXIT_CRITICAL(&s_cmd_mux);
  if (!pending) return;

  switch (cmd[0]) {
    case CMD_SET_TIME:
      if (len >= 9) {
        uint16_t year = cmd[1] | (cmd[2] << 8);
        if (rtc_service_set_from_phone(year, cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8])) {
          char hhmm[6];
          snprintf(hhmm, sizeof(hhmm), "%02u:%02u", cmd[5], cmd[6]);
          g_loggingStatus.pushEvent(hhmm, "Saat telefondan ayarlandi");
        }
      }
      break;
    case CMD_LOG_NOW:
      logging_service_request_write_now();
      break;
    case CMD_BRIGHTNESS:
      if (len >= 2) system_service_set_brightness(cmd[1] > 100 ? 100 : cmd[1]);
      break;
    case CMD_LOGGING:
      if (len >= 2) {
        if (cmd[1]) logging_service_start();
        else logging_service_stop();
      }
      break;
    default:
      Serial.printf("[BLE] Bilinmeyen komut 0x%02X\n", cmd[0]);
      break;
  }
}

// v2 sensor paketi (20 bayt, little-endian):
//  0 surum=2
//  1 bit0-4 sicaklik/nem/gerilim/akim/guc gecerli, bit5 demo, bit6 Wi-Fi, bit7 MQTT
//  2 bit0 SD, bit1 RTC, bit2 kayit acik, bit3 pil gecerli, bit4 son kayit basarili
//  3 pil %
//  4 sicaklik x100 (int16)     6 nem x100 (uint16)
//  8 gerilim x100 (uint16)    10 akim x100 (int16)
// 12 guc x10 (int32)          16 kayit satir sayisi (uint16, doyar)
// 18 parlaklik %              19 en yeni sensor verisinin yasi (sn, 255'te doyar)
static void send_sensors(uint32_t now) {
  const SensorValues &s = g_sensorValues;
  const SystemStatus &st = g_systemStatus;
  uint8_t p[20] = {2};
  p[1] = (s.temperatureValid ? 1 : 0) | (s.humidityValid ? 2 : 0) | (s.voltageValid ? 4 : 0) |
         (s.currentValid ? 8 : 0) | (s.powerValid ? 16 : 0) | (s.demoMode ? 32 : 0) |
         (st.wifiConnected ? 64 : 0) | (st.mqttConnected ? 128 : 0);
  p[2] = (st.sdMounted ? 1 : 0) | (st.rtcValid ? 2 : 0) | (g_loggingStatus.enabled ? 4 : 0) |
         (st.batteryValid ? 8 : 0) | (g_loggingStatus.lastWriteSuccessful ? 16 : 0);
  p[3] = st.batteryValid ? (uint8_t)constrain(st.batteryPercent, 0, 100) : 0;
  put16(p+4, scaled(s.temperature, 100));
  put16(p+6, scaledU16(s.humidity, 100));
  put16(p+8, scaledU16(s.voltage, 100));
  put16(p+10, scaled(s.current, 100));
  put32(p+12, isfinite(s.power) ? (int32_t)lroundf(fmaxf(-2e8f, fminf(2e8f, s.power * 10))) : 0);
  put16(p+16, g_loggingStatus.rowCount > 65535 ? 65535 : g_loggingStatus.rowCount);
  p[18] = st.brightnessPercent;
  uint32_t newest = max(max(max(s.lastTemperatureUpdateMs, s.lastHumidityUpdateMs),
                            max(s.lastVoltageUpdateMs, s.lastCurrentUpdateMs)), s.lastPowerUpdateMs);
  uint32_t age = newest == 0 ? 255 : (now - newest) / 1000;
  p[19] = age > 255 ? 255 : age;
  s_sensors->setValue(p, sizeof(p));
  s_sensors->notify();
}

void ble_service_loop() {
  if (s_restart.exchange(false)) BLEDevice::startAdvertising();
  g_systemStatus.bleConnected = s_connected.load();
  process_command();
  uint32_t now = millis();
  if (!s_data || !s_connected.load()) return;
  if (now - s_last_sensors >= 1000) {
    s_last_sensors = now;
    send_sensors(now);
  }
  if (now - s_last < 200) return;
  s_last = now;
  getGyroscope();
  // 20 bytes fit the default ATT MTU; no fragmentation/MTU negotiation needed.
  // v1, flags(battery valid), uptime LE32, accel milli-g x3,
  // gyro centi-degrees/s x3, battery millivolts LE16.
  uint8_t p[20] = {1, (uint8_t)(g_systemStatus.batteryValid ? 1 : 0)};
  for (int i = 0; i < 4; ++i) p[2+i] = (now >> (8*i)) & 255;
  put16(p+6, scaled(g_motionStatus.accelX, 1000));
  put16(p+8, scaled(g_motionStatus.accelY, 1000));
  put16(p+10, scaled(g_motionStatus.accelZ, 1000));
  // Vendor readings are updated on this same task immediately before us.
  put16(p+12, scaled(Gyro.x, 100));
  put16(p+14, scaled(Gyro.y, 100));
  put16(p+16, scaled(Gyro.z, 100));
  put16(p+18, g_systemStatus.batteryValid ? scaled(g_systemStatus.batteryVoltage, 1000) : 0);
  s_data->setValue(p, sizeof(p));
  s_data->notify();
}
