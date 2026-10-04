#pragma once
#include <lvgl.h>

// Basit tam ekran modal/alt ekran yardimcisi.
// Ayrinti pencereleri (Wi-Fi, MQTT, SD, menu alt ekranlari) bunu kullanir.

// Yeni bir modal acar (varsa oncekini kapatir), baslik + GERI dugmesi ekler
// ve icerik icin bos bir konteyner dondurur (cagiran taraf bu konteynere
// kendi etiket/dugmelerini ekler).
lv_obj_t *ui_modal_open(const char *title);

// Acik modali kapatir. Guvenlidir; modal acik degilse hicbir sey yapmaz.
void ui_modal_close();

// Su anda bir modalin acik olup olmadigini bildirir (periyodik guncelleme
// fonksiyonlarinin, gorunmeyen bir modalin icerigini bosuna guncellememesi icin).
bool ui_modal_is_open();
