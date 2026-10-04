#include "mqtt_service.h"
#include "app_state.h"
#include "app_config.h"
#include "credentials.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <stdlib.h>

static WiFiClient s_wifi_client;
static PubSubClient s_mqtt_client(s_wifi_client);

static uint32_t s_next_attempt_ms = 0;
static uint32_t s_backoff_ms = APP_MQTT_RETRY_MIN_MS;
static bool s_user_disabled = false;
static volatile bool s_connect_requested = false;
static volatile bool s_disconnect_requested = false;

static void apply_sensor_value(const char *topic, float value, bool *validFlag, float *valueField, uint32_t *lastUpdateField)
{
  (void)topic;
  *valueField = value;
  *validFlag = true;
  *lastUpdateField = millis();
  if (g_sensorValues.demoMode) {
    Serial.printf("[MQTT] Ilk gercek veri alindi (%s = %.2f), demo modu kapatildi\r\n", topic, value);
  }
  g_sensorValues.demoMode = false; // gercek veri geldi, demo uretimini durdur
}

static void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  char buf[32];
  if (length >= sizeof(buf)) {
    length = sizeof(buf) - 1;
  }
  memcpy(buf, payload, length);
  buf[length] = '\0';

  char *endptr = NULL;
  float value = strtof(buf, &endptr);
  bool converted = (endptr != buf) && (endptr != NULL) && !isnan(value) && !isinf(value);

  strncpy(g_systemStatus.mqttLastTopic, topic, sizeof(g_systemStatus.mqttLastTopic) - 1);
  g_systemStatus.mqttLastTopic[sizeof(g_systemStatus.mqttLastTopic) - 1] = '\0';
  g_systemStatus.mqttLastMessageMs = millis();

  if (!converted) {
    Serial.printf("[MQTT] Gecersiz payload, eski deger korunuyor: %s = '%s'\r\n", topic, buf);
    return;
  }

  if (strcmp(topic, MQTT_TOPIC_TEMPERATURE) == 0) {
    apply_sensor_value(topic, value, &g_sensorValues.temperatureValid, &g_sensorValues.temperature, &g_sensorValues.lastTemperatureUpdateMs);
  } else if (strcmp(topic, MQTT_TOPIC_HUMIDITY) == 0) {
    apply_sensor_value(topic, value, &g_sensorValues.humidityValid, &g_sensorValues.humidity, &g_sensorValues.lastHumidityUpdateMs);
  } else if (strcmp(topic, MQTT_TOPIC_VOLTAGE) == 0) {
    apply_sensor_value(topic, value, &g_sensorValues.voltageValid, &g_sensorValues.voltage, &g_sensorValues.lastVoltageUpdateMs);
  } else if (strcmp(topic, MQTT_TOPIC_CURRENT) == 0) {
    apply_sensor_value(topic, value, &g_sensorValues.currentValid, &g_sensorValues.current, &g_sensorValues.lastCurrentUpdateMs);
  } else if (strcmp(topic, MQTT_TOPIC_POWER) == 0) {
    apply_sensor_value(topic, value, &g_sensorValues.powerValid, &g_sensorValues.power, &g_sensorValues.lastPowerUpdateMs);
  }
}

static void subscribe_all_topics()
{
  s_mqtt_client.subscribe(MQTT_TOPIC_TEMPERATURE);
  s_mqtt_client.subscribe(MQTT_TOPIC_HUMIDITY);
  s_mqtt_client.subscribe(MQTT_TOPIC_VOLTAGE);
  s_mqtt_client.subscribe(MQTT_TOPIC_CURRENT);
  s_mqtt_client.subscribe(MQTT_TOPIC_POWER);
}

void mqtt_service_init()
{
  s_mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  s_mqtt_client.setCallback(mqtt_callback);

  strncpy(g_systemStatus.mqttHost, MQTT_HOST, sizeof(g_systemStatus.mqttHost) - 1);
  g_systemStatus.mqttPort = MQTT_PORT;

  uint64_t chipId = ESP.getEfuseMac();
  snprintf(g_systemStatus.mqttClientId, sizeof(g_systemStatus.mqttClientId), MQTT_CLIENT_ID_PREFIX "%04X", (unsigned int)(chipId & 0xFFFF));
}

void mqtt_service_loop()
{
  if (s_disconnect_requested) {
    s_disconnect_requested = false;
    s_user_disabled = true;
    s_mqtt_client.disconnect();
  }
  if (s_connect_requested) {
    s_connect_requested = false;
    s_user_disabled = false;
    s_backoff_ms = APP_MQTT_RETRY_MIN_MS;
    s_next_attempt_ms = millis();
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (s_mqtt_client.connected()) {
      s_mqtt_client.disconnect();
    }
    g_systemStatus.mqttConnected = false;
    return;
  }

  if (s_mqtt_client.connected()) {
    g_systemStatus.mqttConnected = true;
    s_mqtt_client.loop();
    return;
  }

  g_systemStatus.mqttConnected = false;
  if (s_user_disabled) {
    return;
  }
  if (millis() < s_next_attempt_ms) {
    return;
  }

  Serial.printf("[MQTT] Baglaniliyor: %s:%u\r\n", g_systemStatus.mqttHost, g_systemStatus.mqttPort);
  bool ok;
  if (strlen(MQTT_USERNAME) > 0) {
    ok = s_mqtt_client.connect(g_systemStatus.mqttClientId, MQTT_USERNAME, MQTT_PASSWORD);
  } else {
    ok = s_mqtt_client.connect(g_systemStatus.mqttClientId);
  }

  if (ok) {
    Serial.println("[MQTT] Baglandi");
    s_backoff_ms = APP_MQTT_RETRY_MIN_MS;
    subscribe_all_topics();
  } else {
    g_systemStatus.mqttReconnectCount++;
    s_next_attempt_ms = millis() + s_backoff_ms;
    s_backoff_ms = min<uint32_t>(s_backoff_ms * 2, APP_MQTT_RETRY_MAX_MS);
    Serial.printf("[MQTT] Baglanti basarisiz (state=%d), %lu ms sonra tekrar denenecek\r\n", s_mqtt_client.state(), (unsigned long)s_backoff_ms);
  }
}

void mqtt_service_request_connect()
{
  s_connect_requested = true;
}

void mqtt_service_request_disconnect()
{
  s_disconnect_requested = true;
}
