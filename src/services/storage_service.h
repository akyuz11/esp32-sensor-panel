#pragma once

// TF/microSD kart baglama ve kapasite bilgisini g_systemStatus'a yazar.
// CSV kayit islevleri Asama 5'te (storage_service_write_row vb.) eklenecek.

void storage_service_init();

// Periyodik olarak (ör. birkac saniyede bir) cagrilir; kart cikarilmis/
// yeniden takilmis olabilir, mevcut durumu tazeler.
void storage_service_update();

bool storage_service_is_mounted();

// Ders denemesi: /deneme.txt dosyasina "Acilis #N" satiri ekler, sonra
// dosyanin tamamini geri okuyup seri monitore yazar. Bloklayici SD islemi
// yaptigi icin yalnizca setup() icinden (LVGL baslamadan) cagrilir.
void storage_service_run_boot_test();
