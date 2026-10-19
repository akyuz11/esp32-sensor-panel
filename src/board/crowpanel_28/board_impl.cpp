// board.h'nin Elecrow CrowPanel Advance 2.8" (V1.1/V1.2) uygulamasi.
//
// Ekran (ST7789, SPI) ve dokunmatik (FT6336U, I2C) Elecrow'un resmi
// orneklerindeki LovyanGFX ayariyla (vendor/LovyanGFX_Driver.h, degistirilmez)
// surulur. Elecrow ornekleri LVGL 9 kullanir; bu proje LVGL 8 oldugu icin
// LVGL baglantisi (draw buffer, flush, touch read, tick) burada LVGL 8
// API'siyle yazildi. Pinler: Elecrow-RD/CrowPanel-Advance-2.8-HMI-ESP32-S3...
// example/V1.1_and_V1.2/Arduino_Code (lesson-03 LCD/touch, lesson-04 SD).
//
// Kartta pil ve RTC donanimi var ama Elecrow orneklerinde okuma kodu/pinleri
// yok; bu yuzden simdilik:
//   - pil: yok kabul edilir (board_has_battery() = false)
//   - RTC: ESP32 sistem saati kullanilir (NTP veya telefon ile ayarlanir,
//     guc kesilince kaybolur). Acilista I2C taramasi yapilip bulunan adresler
//     seri monitore yazilir; RTC cipi belirlenince buraya eklenecek.

#include "board.h"

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <time.h>

#include "vendor/LovyanGFX_Driver.h"

#define LCD_H_RES        320
#define LCD_V_RES        240

#define BACKLIGHT_PIN    38
#define BACKLIGHT_FREQ   20000
#define BACKLIGHT_BITS   10

#define TOUCH_SDA_PIN    15
#define TOUCH_SCL_PIN    16

#define SD_MOSI_PIN      6
#define SD_MISO_PIN      4
#define SD_SCK_PIN       5
#define SD_CS_PIN        7
#define SD_SPI_FREQ      20000000

#define LVGL_TICK_MS     2
#define LVGL_BUF_LINES   40

static LGFX s_gfx;
static SPIClass s_sd_spi(HSPI);
static bool s_backlight_attached = false;

const char *board_name() { return "Elecrow CrowPanel Advance 2.8"; }
bool board_has_battery() { return false; }
bool board_has_rtc() { return false; }
bool board_has_imu() { return false; }

// I2C hattindaki cihazlari listeler (dokunmatik 0x38; RTC'nin adresi buradan
// ogrenilecek). Wire, LovyanGFX dokunmatigi baslamadan once acilip kapatilir.
static void scan_i2c()
{
  Wire.begin(TOUCH_SDA_PIN, TOUCH_SCL_PIN);
  Serial.print("[BOARD] I2C cihazlari:");
  int found = 0;
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" 0x%02X", addr);
      found++;
    }
  }
  Serial.println(found ? "" : " (yok)");
  Wire.end();
}

void board_init()
{
  // Elecrow orneklerindeki acilis sirasi (lesson-03): dokunmatik hattinin
  // reset/adres secimi icin IO1/IO2 darbesi.
  pinMode(18, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  delay(20);
  digitalWrite(2, HIGH);
  delay(100);
  pinMode(1, INPUT);

  scan_i2c();

  // Ekran SD'den once acilmaz (main.cpp sirasi); arka isik simdiden kapali.
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, LOW);
}

// --- LVGL 8 baglantisi ---

static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static lv_indev_drv_t s_indev_drv;

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  // LV_COLOR_16_SWAP 0: lv_color_t yerel RGB565; LovyanGFX bayt sirasini
  // kendisi ayarlar. Senkron gonderim: buffer, fonksiyon donunce serbest.
  s_gfx.pushImage(area->x1, area->y1, w, h, (lgfx::rgb565_t *)color_p);
  lv_disp_flush_ready(drv);
}

static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  uint16_t x, y;
  if (s_gfx.getTouch(&x, &y)) {
    // Elecrow ornegindeki eksen eslemesi (lesson-03 my_touchpad_read).
    data->state = LV_INDEV_STATE_PR;
    data->point.x = LCD_H_RES - x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

static void lvgl_tick_cb(void *arg)
{
  lv_tick_inc(LVGL_TICK_MS);
}

void board_display_init()
{
  s_gfx.init();
  s_gfx.fillScreen(TFT_BLACK);
  Serial.printf("[BOARD] LCD %dx%d\r\n", (int)s_gfx.width(), (int)s_gfx.height());

  lv_init();

  size_t buf_px = LCD_H_RES * LVGL_BUF_LINES;
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (buf1 == NULL || buf2 == NULL) {
    // Dahili RAM yetmezse PSRAM'e dus (daha yavas ama calisir).
    if (buf1 == NULL) buf1 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (buf2 == NULL) buf2 = (lv_color_t *)heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  }
  lv_disp_draw_buf_init(&s_draw_buf, buf1, buf2, buf_px);

  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res = LCD_H_RES;
  s_disp_drv.ver_res = LCD_V_RES;
  s_disp_drv.flush_cb = lvgl_flush_cb;
  s_disp_drv.draw_buf = &s_draw_buf;
  lv_disp_drv_register(&s_disp_drv);

  lv_indev_drv_init(&s_indev_drv);
  s_indev_drv.type = LV_INDEV_TYPE_POINTER;
  s_indev_drv.read_cb = lvgl_touch_cb;
  lv_indev_drv_register(&s_indev_drv);

  const esp_timer_create_args_t tick_args = {
      .callback = &lvgl_tick_cb,
      .name = "lvgl_tick"};
  esp_timer_handle_t tick_timer = NULL;
  esp_timer_create(&tick_args, &tick_timer);
  esp_timer_start_periodic(tick_timer, LVGL_TICK_MS * 1000);

  s_backlight_attached = ledcAttach(BACKLIGHT_PIN, BACKLIGHT_FREQ, BACKLIGHT_BITS);
  board_set_backlight(50);
}

void board_ui_loop()
{
  lv_timer_handler();
}

void board_poll()
{
  // Yoklanacak kart donanimi yok (pil/RTC/IMU okunmuyor).
}

void board_set_backlight(uint8_t percent)
{
  if (percent > 100) percent = 100;
  if (s_backlight_attached) {
    ledcWrite(BACKLIGHT_PIN, (uint32_t)percent * ((1 << BACKLIGHT_BITS) - 1) / 100);
  } else {
    digitalWrite(BACKLIGHT_PIN, percent > 0 ? HIGH : LOW);
  }
}

float board_battery_volts() { return 0.0f; }

// --- Zaman: ESP32 sistem saati (RTC cipi eklenene kadar) ---

void board_rtc_now(BoardDateTime *out)
{
  time_t now = time(NULL);
  struct tm t;
  localtime_r(&now, &t);
  out->year = t.tm_year + 1900;
  out->month = t.tm_mon + 1;
  out->day = t.tm_mday;
  out->dotw = t.tm_wday;
  out->hour = t.tm_hour;
  out->minute = t.tm_min;
  out->second = t.tm_sec;
}

bool board_rtc_lost_power()
{
  // Sistem saati hic ayarlanmadiysa 1970'tedir: gecersiz say.
  return time(NULL) < 1704067200; // 2024-01-01
}

void board_rtc_write(const BoardDateTime &dt)
{
  struct tm t = {};
  t.tm_year = dt.year - 1900;
  t.tm_mon = dt.month - 1;
  t.tm_mday = dt.day;
  t.tm_hour = dt.hour;
  t.tm_min = dt.minute;
  t.tm_sec = dt.second;
  t.tm_isdst = -1;
  struct timeval tv = {mktime(&t), 0};
  settimeofday(&tv, NULL);
}

bool board_imu_accel(BoardVec3 *out) { return false; }

bool board_imu_gyro(BoardVec3 *out)
{
  out->x = out->y = out->z = 0;
  return false;
}

// --- SD kart (SPI, ayri HSPI hatti) ---

bool board_sd_begin()
{
  s_sd_spi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, s_sd_spi, SD_SPI_FREQ)) {
    s_sd_spi.end();
    return false;
  }
  return true;
}

void board_sd_end()
{
  SD.end();
  s_sd_spi.end();
}

bool board_sd_card_present() { return SD.cardType() != CARD_NONE; }
uint64_t board_sd_total_bytes() { return SD.totalBytes(); }
uint64_t board_sd_used_bytes() { return SD.usedBytes(); }
fs::FS &board_sd_fs() { return SD; }
