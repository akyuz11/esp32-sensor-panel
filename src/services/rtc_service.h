#pragma once
#include <Arduino.h>

// PCF85063 RTC durumunu okuyup g_systemStatus/saat metinlerini gunceller.
// PCF85063_Init() ve PCF85063_Loop() Waveshare'in resmi surucusunden oldugu
// gibi kullanilir (main.cpp -> Driver_Init/Driver_Loop); bu servis yalnizca
// o surucunun urettigi veriyi yorumlar ve NTP senkronizasyonunu yonetir.

void rtc_service_update();

// "HH:MM" ve "GG.AA.YYYY" bicimlerinde arabelleklenmis dizeler dondurur.
// RTC gecerli degilse ikisi de bos dizedir.
const char *rtc_service_time_text();
const char *rtc_service_date_text();

// Wi-Fi baglandiginda dogrudan cagrilir (arka plan servis gorevinde calisir,
// birkac saniyeye kadar bloklayabilir). Basariliysa RTC'yi de gunceller ve
// true doner; basarisizsa RTC'ye dokunmaz.
bool rtc_service_sync_from_ntp();

// "RTC'YE YAZ" dugmesi: bu oturumda en az bir kez basarili NTP senkronu
// yapildiysa, ESP32'nin sistem saatini tekrar RTC'ye yazar.
bool rtc_service_write_system_time_to_rtc();

// Telefon uygulamasindan (BLE) gelen yerel saati RTC'ye yazar. Yalnizca
// Driver_Loop'tan cagrilir (I2C). Mantiksiz tarih/saat reddedilir.
bool rtc_service_set_from_phone(uint16_t year, uint8_t month, uint8_t day,
                                uint8_t hour, uint8_t minute, uint8_t second,
                                uint8_t dotw);

uint32_t rtc_service_last_ntp_sync_ms();
bool rtc_service_ntp_ever_synced();

// LVGL dokunma geri cagrilarindan cagirmak icin GUVENLI istek fonksiyonlari:
// gercek (bloklayabilen) islem arka plan gorevinde (rtc_service_process_requests)
// calisir, UI donmaz.
void rtc_service_request_ntp_sync();
void rtc_service_request_write_to_rtc();
void rtc_service_process_requests();
