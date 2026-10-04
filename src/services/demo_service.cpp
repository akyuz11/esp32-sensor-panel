#include "demo_service.h"
#include "app_state.h"
#include <Arduino.h>
#include <math.h>

static uint32_t s_last_update_ms = 0;
#define DEMO_UPDATE_PERIOD_MS 2000

void demo_service_loop()
{
  if (!g_sensorValues.demoMode) {
    return;
  }

  uint32_t now = millis();
  if (now - s_last_update_ms < DEMO_UPDATE_PERIOD_MS) {
    return;
  }
  s_last_update_ms = now;

  float t = now / 1000.0f;

  g_sensorValues.temperature = 24.6f + sinf(t / 20.0f) * 1.5f;
  g_sensorValues.humidity = 48.0f + sinf(t / 25.0f + 1.0f) * 4.0f;
  g_sensorValues.voltage = 18.4f + sinf(t / 30.0f + 2.0f) * 0.6f;
  g_sensorValues.current = 6.85f + sinf(t / 18.0f + 0.5f) * 0.4f;
  g_sensorValues.power = g_sensorValues.voltage * g_sensorValues.current;

  g_sensorValues.temperatureValid = true;
  g_sensorValues.humidityValid = true;
  g_sensorValues.voltageValid = true;
  g_sensorValues.currentValid = true;
  g_sensorValues.powerValid = true;

  g_sensorValues.lastTemperatureUpdateMs = now;
  g_sensorValues.lastHumidityUpdateMs = now;
  g_sensorValues.lastVoltageUpdateMs = now;
  g_sensorValues.lastCurrentUpdateMs = now;
  g_sensorValues.lastPowerUpdateMs = now;
}
