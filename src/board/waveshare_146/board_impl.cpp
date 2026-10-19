// board.h'nin Waveshare ESP32-S3-Touch-LCD-1.46 uygulamasi.
// vendor/ altindaki dosyalar Waveshare'in resmi ornegidir ve degistirilmez;
// buradaki kod yalnizca onlari board.h arayuzune baglar.

#include "board.h"

#include "vendor/I2C_Driver.h"
#include "vendor/TCA9554PWR.h"
#include "vendor/Display_SPD2010.h"
#include "vendor/Touch_SPD2010.h"
#include "vendor/LVGL_Driver.h"
#include "vendor/BAT_Driver.h"
#include "vendor/RTC_PCF85063.h"
#include "vendor/Gyro_QMI8658.h"
#include "vendor/PWR_Key.h"
#include "vendor/SD_Card.h"

const char *board_name() { return "Waveshare ESP32-S3-Touch-LCD-1.46"; }
bool board_has_battery() { return true; }
bool board_has_rtc() { return true; }
bool board_has_imu() { return true; }

// Waveshare'in resmi ornegindeki baslatma sirasi degistirilmeden kullanilir.
void board_init()
{
  Flash_test();
  PWR_Init();
  BAT_Init();
  I2C_Init();
  TCA9554PWR_Init(0x00);
  Backlight_Init();
  Set_Backlight(50); // 0~100
  PCF85063_Init();
  QMI8658_Init();
}

void board_display_init()
{
  LCD_Init();
  Lvgl_Init();
}

void board_ui_loop()
{
  Lvgl_Loop();
}

void board_poll()
{
  PWR_Loop();
  QMI8658_Loop();
  PCF85063_Loop();
  BAT_Get_Volts();
}

void board_set_backlight(uint8_t percent)
{
  Set_Backlight(percent);
}

float board_battery_volts()
{
  // BAT_Get_Volts() board_poll() icinde cagriliyor; burada en son deger.
  return BAT_analogVolts;
}

void board_rtc_now(BoardDateTime *out)
{
  out->year = datetime.year;
  out->month = datetime.month;
  out->day = datetime.day;
  out->dotw = datetime.dotw;
  out->hour = datetime.hour;
  out->minute = datetime.minute;
  out->second = datetime.second;
}

bool board_rtc_lost_power()
{
  // PCF85063 saniye kaydinin bit7'si "OS" (Osilator Durdu) bayragidir:
  // guc kesintisi/pil bosalmasi sonrasi 1 olur ve zamanin gecersiz
  // olabilecegini gosterir. Vendor'un PCF85063_Read_Time() fonksiyonu bu
  // biti maskeleyip attigi icin burada dogrudan kendimiz okuyoruz
  // (vendor fonksiyonunu degistirmeden).
  // Not: vendor I2C_Read() basarida false (0), hatada true (-1) doner.
  uint8_t raw = 0;
  if (!I2C_Read(PCF85063_ADDRESS, RTC_SECOND_ADDR, &raw, 1)) {
    return (raw & 0x80) != 0;
  }
  return true; // Okuma basarisizsa gecersiz kabul et.
}

void board_rtc_write(const BoardDateTime &dt)
{
  datetime_t v;
  v.year = dt.year;
  v.month = dt.month;
  v.day = dt.day;
  v.dotw = dt.dotw;
  v.hour = dt.hour;
  v.minute = dt.minute;
  v.second = dt.second;
  // Saniye kaydinin yazilmasi OS bayragini da temizler.
  PCF85063_Set_All(v);
}

bool board_imu_accel(BoardVec3 *out)
{
  // Accel, board_poll() icindeki QMI8658_Loop() ile guncellenir.
  out->x = Accel.x;
  out->y = Accel.y;
  out->z = Accel.z;
  return true;
}

bool board_imu_gyro(BoardVec3 *out)
{
  getGyroscope();
  out->x = Gyro.x;
  out->y = Gyro.y;
  out->z = Gyro.z;
  return true;
}

bool board_sd_begin()
{
  SD_Init(); // EXIO D3 etkinlestirme + SD_MMC.begin
  return board_sd_card_present();
}

void board_sd_end() { SD_MMC.end(); }
bool board_sd_card_present() { return SD_MMC.cardType() != CARD_NONE; }
uint64_t board_sd_total_bytes() { return SD_MMC.totalBytes(); }
uint64_t board_sd_used_bytes() { return SD_MMC.usedBytes(); }
fs::FS &board_sd_fs() { return SD_MMC; }
