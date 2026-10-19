#include "battery_service.h"
#include "app_state.h"
#include "board.h"

// Basit, dogrusal olmayan Li-Po gerilim -> yuzde tablosu (buyukten kucuge).
struct BatteryPoint {
  float volts;
  int percent;
};

static const BatteryPoint kBatteryTable[] = {
    {4.20f, 100},
    {4.10f, 90},
    {4.00f, 80},
    {3.90f, 65},
    {3.80f, 45},
    {3.70f, 25},
    {3.60f, 10},
    {3.40f, 0},
};
static const int kBatteryTableLen = sizeof(kBatteryTable) / sizeof(kBatteryTable[0]);

static int voltage_to_percent(float volts)
{
  if (volts >= kBatteryTable[0].volts) {
    return 100;
  }
  if (volts <= kBatteryTable[kBatteryTableLen - 1].volts) {
    return 0;
  }
  for (int i = 0; i < kBatteryTableLen - 1; i++) {
    float v_hi = kBatteryTable[i].volts;
    float v_lo = kBatteryTable[i + 1].volts;
    if (volts <= v_hi && volts >= v_lo) {
      float ratio = (volts - v_lo) / (v_hi - v_lo);
      int p_hi = kBatteryTable[i].percent;
      int p_lo = kBatteryTable[i + 1].percent;
      return p_lo + (int)((p_hi - p_lo) * ratio);
    }
  }
  return 0;
}

void battery_service_update()
{
  // Pil ADC'si Driver_Loop icinde (100ms'de bir) board_poll() ile zaten
  // okunuyor; burada yalnizca en son okunan degeri yorumluyoruz.
  float volts = board_battery_volts();

  // Kartta gercek bir "pil takili mi" pini yok. BAT_ADC_PIN, pilin dogrudan
  // gerilim boluculu girisidir; pil yokken bu hat genelde cok dusuk/gurultulu
  // okunur. Bu esikler yaklasiktir ve yalnizca kaba bir ayrim icin kullanilir.
  if (volts < 1.5f) {
    g_systemStatus.batteryValid = false;
    g_systemStatus.batteryVoltage = 0.0f;
    g_systemStatus.batteryPercent = 0;
  } else if (volts < 3.4f) {
    // Gecerli bir Li-Po araligi disinda: olcum guvenilir degil.
    g_systemStatus.batteryValid = false;
    g_systemStatus.batteryVoltage = volts;
    g_systemStatus.batteryPercent = 0;
  } else {
    g_systemStatus.batteryValid = true;
    g_systemStatus.batteryVoltage = volts;
    g_systemStatus.batteryPercent = voltage_to_percent(volts);
  }
}
