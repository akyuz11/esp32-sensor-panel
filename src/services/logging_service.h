#pragma once

// TF karta /logs/YYYY-MM-DD.csv seklinde periyodik (varsayilan 60 sn) veri
// kaydi. SD kart yoksa veya RTC gecerli degilse sessizce atlar (cihaz
// cokmez, sahte tarihli dosya olusturmaz).

void logging_service_init();

// Driver_Loop'tan sik cagrilir (ör. 1 sn'de bir); dahili araligina gore
// kendisi tetiklenir.
void logging_service_loop();

void logging_service_start();
void logging_service_stop();

// "SIMDI KAYDET" dugmesi: LVGL geri cagrisindan GUVENLE cagrilabilir,
// gercek dosya yazimi arka plan gorevinde yapilir.
void logging_service_request_write_now();

// "KAYDI IZLE" ekrani: LVGL geri cagrisindan GUVENLE cagrilabilir. En yeni
// CSV dosyasinin son satirlari arka plan gorevinde okunup g_logView'a
// (en yeni kayit basta olacak sekilde) yazilir.
void logging_service_request_view_reload();
