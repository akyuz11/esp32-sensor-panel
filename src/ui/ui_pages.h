#pragma once
#include <lvgl.h>

// Dort ana sayfa: DURUM, VERILER, MENU, KAYIT.
// Her sayfanin *_create() fonksiyonu bir tile (tam ekran 412x412 alan) alir
// ve icerigini olusturur. *_update() fonksiyonlari periyodik zamanlayicidan
// (yaklasik 1 saniyede bir) cagrilir ve yalnizca degisen etiket/gostergeleri
// gunceller; sayfa yeniden olusturulmaz.

void ui_page_status_create(lv_obj_t *tile);
void ui_page_status_update();

void ui_page_data_create(lv_obj_t *tile);
void ui_page_data_update();

void ui_page_menu_create(lv_obj_t *tile);
void ui_page_menu_update();

void ui_page_log_create(lv_obj_t *tile);
void ui_page_log_update();
