#pragma once

// ---------------------------------------------------------------------------
// Kart soyutlama katmani.
//
// main.cpp ve servisler donanima YALNIZCA bu arayuz uzerinden erisir; hicbiri
// vendor surucu basliklarini (src/board/<kart>/vendor/) dogrudan include
// etmez. Her kart bu arayuzu src/board/<kart>/board_impl.cpp icinde kendi
// vendor suruculeriyle uygular. Hangi kartin derlenecegini platformio.ini'deki
// build_src_filter secer.
//
// Bir kartta olmayan donanim icin ilgili board_has_*() false doner ve diger
// fonksiyonlar "yok" degeri (false / 0) dondurur; servisler buna gore davranir.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <FS.h>

struct BoardDateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t dotw;   // 0 = Pazar
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

struct BoardVec3 {
  float x;
  float y;
  float z;
};

// --- Kart bilgisi / yetenekler ---
const char *board_name();
bool board_has_battery();
bool board_has_rtc();
bool board_has_imu();

// --- Yasam dongusu ---
// SD ve ekrandan once: guc tutma, I2C, IO genisletici, arka isik, RTC, IMU.
void board_init();
// Ekran + dokunmatik + LVGL surucusu (lv_init dahil).
void board_display_init();
// Arduino loop() icinden (UI gorevi) cagrilir: lv_timer_handler.
void board_ui_loop();
// Driver_Loop icinden ~100ms'de bir cagrilir: vendor yoklamalari
// (guc tusu, IMU, RTC, pil ADC).
void board_poll();

// --- Ekran ---
void board_set_backlight(uint8_t percent); // 0..100

// --- Pil ---
// Son olculen pil gerilimi (V). Pil olcumu olmayan kartta 0.
float board_battery_volts();

// --- RTC ---
// board_poll() ile en son okunan zaman (I2C erisimi yapmaz).
void board_rtc_now(BoardDateTime *out);
// RTC osilatoru durmus / zaman guvenilmez mi? (I2C okumasi yapar.)
// RTC'si olmayan ya da okunamayan kartta true.
bool board_rtc_lost_power();
void board_rtc_write(const BoardDateTime &dt);

// --- IMU ---
// En son yoklanan ivme (g). I2C erisimi yapmaz.
bool board_imu_accel(BoardVec3 *out);
// Jiroskobu o anda okur (dps). I2C erisimi yapar.
bool board_imu_gyro(BoardVec3 *out);

// --- SD kart ---
bool board_sd_begin();
void board_sd_end();
bool board_sd_card_present();
uint64_t board_sd_total_bytes();
uint64_t board_sd_used_bytes();
fs::FS &board_sd_fs();
