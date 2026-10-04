#pragma once
#include <lvgl.h>

// Uygulama kabuğu: tileview (4 sayfa), alt gezinme çubuğu ve periyodik
// arayüz güncelleme zamanlayıcısını kurar. Bir kez, LVGL başlatıldıktan
// sonra çağrılır.
void ui_app_init();

// Son dokunmadan bu yana gecen sure (ms). Asama 3'teki ekran uyku
// zamanlayicisi bunu kullanacak. LVGL'in kendi etkinlik izleyicisini sarar.
uint32_t ui_app_inactivity_ms();
