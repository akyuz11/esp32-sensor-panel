#pragma once

// Bloklamayan Wi-Fi baglanti yonetimi: baslangicta kisa aralik, sonraki
// denemelerde APP_WIFI_RETRY_MAX_MS'e kadar geri cekilme. g_systemStatus
// alanlarini gunceller. Baglanti kurulduginda RTC servisini NTP ile
// esitlemeyi bir kez dener.

void wifi_service_init();

// main loop / servis gorevinden sik sik (ör. her dongude) cagrilir;
// engellemeyen durum makinesi calistirir.
void wifi_service_loop();

// MENU sayfasindaki "YENIDEN BAGLAN" dugmesi: geri cekilmeyi sifirlayip
// hemen yeni bir baglanti denemesi ister (gercek islem servis gorevinde
// yapilir, LVGL geri cagrisi icinde degil).
void wifi_service_request_reconnect();

// "AGLARI TARA" dugmesi: engellemeyen bir tarama baslatir. Sonuc hazir
// oldugunda wifi_service_scan_result_count() > -1 doner.
void wifi_service_request_scan();
int wifi_service_scan_result_count(); // -1 = tarama devam ediyor / baslamadi
