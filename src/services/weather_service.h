#pragma once

// Open-Meteo (api anahtari gerektirmez) uzerinden periyodik hava durumu
// cekimi; sonucu g_weatherStatus'a yazar. Diger servisler gibi bloklayici
// HTTP istegi yalnizca Driver_Loop icinden cagrilan _loop() fonksiyonunda
// yapilir, LVGL/UI thread'inden asla dogrudan cagrilmaz.

void weather_service_init();

// Driver_Loop icinden periyodik cagrilir; zamani gelince (veya elle istek
// yapildiysa) bloklayici bir HTTP GET calistirir.
void weather_service_loop();

// LVGL geri cagrilarindan (or. "YENILE" dugmesi) guvenle cagrilabilir;
// bir sonraki weather_service_loop() calismasinda hemen yenileme tetikler.
void weather_service_request_refresh();

// Open-Meteo WMO hava kodunu kisa Turkce metne cevirir (UI'de gosterim icin).
const char *weather_service_code_text(int code);
