#pragma once
#include <lvgl.h>

// ---------------------------------------------------------------------------
// Ekran geometrisi (guvenli icerik alani) ve ortak koyu tema.
// Ekran boyutu kartin platformio.ini env'indeki build_flags'ten gelir
// (-DUI_SCREEN_W / -DUI_SCREEN_H / -DUI_SCREEN_ROUND); verilmezse Waveshare
// 1.46'nin yuvarlak 412x412 ekrani varsayilir.
// ---------------------------------------------------------------------------

#ifndef UI_SCREEN_W
#define UI_SCREEN_W      412
#define UI_SCREEN_H      412
#define UI_SCREEN_ROUND  1
#endif

#if UI_SCREEN_ROUND
// Yuvarlak 412x412 ekran.
#define UI_SAFE_LEFT     38
#define UI_SAFE_RIGHT    374
#define UI_SAFE_TOP      25
#define UI_SAFE_BOTTOM   380

// Nav bar: yuvarlak ekranda dis tuslarin koseleri kesilmesin diye tam genislik
// yerine daraltilmis ve ekran altindan yukari alinmis (tus koseleri merkeze ~200px).
#define UI_NAV_HEIGHT    44
#define UI_NAV_WIDTH     248
#define UI_NAV_BOTTOM    372   // nav barin alt kenarinin y koordinati
#else
// Dikdortgen ekran: kenarlarda yalnizca kucuk bir bosluk, nav bar tam genislik.
#define UI_SAFE_LEFT     6
#define UI_SAFE_RIGHT    (UI_SCREEN_W - 6)
#define UI_SAFE_TOP      4
#define UI_SAFE_BOTTOM   (UI_SCREEN_H - 2)

#define UI_NAV_HEIGHT    36
#define UI_NAV_WIDTH     (UI_SCREEN_W - 12)
#define UI_NAV_BOTTOM    (UI_SCREEN_H - 2)
#endif

#define UI_NAV_TOP       (UI_NAV_BOTTOM - UI_NAV_HEIGHT)

// Kucuk dikdortgen ekran: sayfalar yatay, sikisik duzenle kurulur.
#define UI_COMPACT (!UI_SCREEN_ROUND)

// Yazi tipleri (punto) ve olcek. Yuvarlak ekranda degerler eski sabitlerle
// birebir aynidir; kompakt ekranda bir-iki kademe kucuk.
#if UI_COMPACT
#define UI_FONT_SMALL        (&lv_font_montserrat_14)  // aciklama / satir metni
#define UI_FONT_BUTTON       (&lv_font_montserrat_14)
#define UI_FONT_NAV          (&lv_font_montserrat_14)
#define UI_FONT_TITLE        (&lv_font_montserrat_18)  // modal basligi
#define UI_FONT_PAGE_TITLE   (&lv_font_montserrat_18)  // sayfa basligi
#define UI_FONT_VALUE        (&lv_font_montserrat_28)  // sensor degeri
#define UI_FONT_CLOCK        (&lv_font_montserrat_36)
#define UI_FONT_BIG          (&lv_font_montserrat_20)  // pil yuzdesi, hava sicakligi
#define UI_FONT_ACCENT       (&lv_font_montserrat_14)  // DEMO etiketi
#define UI_FONT_HINT         (&lv_font_montserrat_18)
#define UI_CARD_PAD          6
#define UI_CARD_RADIUS       10
#define UI_BUTTON_HEIGHT     34
#else
#define UI_FONT_SMALL        (&lv_font_montserrat_18)
#define UI_FONT_BUTTON       (&lv_font_montserrat_16)
#define UI_FONT_NAV          (&lv_font_montserrat_16)
#define UI_FONT_TITLE        (&lv_font_montserrat_28)
#define UI_FONT_PAGE_TITLE   (&lv_font_montserrat_26)
#define UI_FONT_VALUE        (&lv_font_montserrat_28)
#define UI_FONT_CLOCK        (&lv_font_montserrat_36)
#define UI_FONT_BIG          (&lv_font_montserrat_26)
#define UI_FONT_ACCENT       (&lv_font_montserrat_18)
#define UI_FONT_HINT         (&lv_font_montserrat_24)
#define UI_CARD_PAD          8
#define UI_CARD_RADIUS       14
#define UI_BUTTON_HEIGHT     44
#endif

// Modal (detay/alt ekran) icerik kutusu.
// Yuvarlak: baslik ustte, GERI altta ortada. Kompakt: GERI basligin solunda,
// icerik basligin altindan ekranin altina kadar.
#if UI_COMPACT
#define UI_MODAL_HEADER_HEIGHT  34
#define UI_MODAL_CONTENT_TOP    (UI_SAFE_TOP + UI_MODAL_HEADER_HEIGHT)
#define UI_MODAL_CONTENT_HEIGHT (UI_SCREEN_H - UI_MODAL_CONTENT_TOP - 4)
#else
#define UI_MODAL_CONTENT_TOP    (UI_SAFE_TOP + 40)
#define UI_MODAL_CONTENT_HEIGHT 220
#endif

// Nav barin ustunde kalan, sayfa icerigi icin kullanilabilir guvenli alan.
#define UI_CONTENT_LEFT    UI_SAFE_LEFT
#define UI_CONTENT_TOP     UI_SAFE_TOP
#define UI_CONTENT_WIDTH   (UI_SAFE_RIGHT - UI_SAFE_LEFT)
#define UI_CONTENT_HEIGHT  (UI_NAV_TOP - UI_SAFE_TOP)

// Renk paleti kartin env'indeki build_flags'ten secilir:
// -DUI_THEME_MONO=1 -> siyah-beyaz (CrowPanel), yoksa "Gece Mavisi".
#ifndef UI_THEME_MONO
#define UI_THEME_MONO 0
#endif

#if UI_THEME_MONO
// Siyah-beyaz: siyah zemin, beyaz yazi, yalnizca gri tonlari. Durum noktalari
// renk yerine parlaklikla ayrilir (bagli = beyaz, uyari = gri, yok = koyu gri);
// durum zaten yanlarindaki yazida da soylenir.
#define UI_COLOR_BG          lv_color_hex(0x000000)
#define UI_COLOR_BG_GRAD_END lv_color_hex(0x000000)
#define UI_COLOR_CARD        lv_color_hex(0x121212)
#define UI_COLOR_CARD_GRAD_END lv_color_hex(0x121212)
#define UI_COLOR_CARD_BORDER lv_color_hex(0x3d3d3d)
#define UI_COLOR_TEXT        lv_color_hex(0xffffff)
#define UI_COLOR_TEXT_DIM    lv_color_hex(0xa3a3a3)
#define UI_COLOR_ACCENT      lv_color_hex(0xffffff)  // arc/slider/vurgu
#define UI_COLOR_ACCENT_FILL lv_color_hex(0x525252)  // buton dolgusu (beyaz yazi okunur)
#define UI_COLOR_GOOD        lv_color_hex(0xffffff)
#define UI_COLOR_WARN        lv_color_hex(0xa3a3a3)
#define UI_COLOR_BAD         lv_color_hex(0x5c5c5c)
#define UI_COLOR_ACCENT_HEX  "ffffff"                // lv_label recolor icin
#else
// "Gece Mavisi" paleti: lacivertten siyaha dikey gradyan zemin, camgobegi vurgu.
#define UI_COLOR_BG          lv_color_hex(0x111a36)  // zemin gradyaninin ust rengi
#define UI_COLOR_BG_GRAD_END lv_color_hex(0x04060d)  // zemin gradyaninin alt rengi
#define UI_COLOR_CARD        lv_color_hex(0x182040)  // kart gradyaninin ust rengi
#define UI_COLOR_CARD_GRAD_END lv_color_hex(0x10162c)
#define UI_COLOR_CARD_BORDER lv_color_hex(0x25304f)
#define UI_COLOR_TEXT        lv_color_hex(0xeaf0ff)
#define UI_COLOR_TEXT_DIM    lv_color_hex(0x8a96b8)
#define UI_COLOR_ACCENT      lv_color_hex(0x22d3ee)  // arc/slider/vurgu (parlak)
#define UI_COLOR_ACCENT_FILL lv_color_hex(0x0e7490)  // buton dolgusu (beyaz yazi okunur)
#define UI_COLOR_GOOD        lv_color_hex(0x34d399)
#define UI_COLOR_WARN        lv_color_hex(0xfbbf24)
#define UI_COLOR_BAD         lv_color_hex(0xf87171)
#define UI_COLOR_ACCENT_HEX  "22d3ee"                // lv_label recolor icin
#endif

// Ekranin genel koyu temasini uygular (gradyan arka plan, varsayilan yazi rengi).
void ui_theme_apply_screen(lv_obj_t *scr);

// Bir nesneye tam ekran zemin gradyanini uygular (tileview, modal kok vb.).
void ui_theme_apply_bg_gradient(lv_obj_t *obj);

// Yuvarlatilmis, koyu bir "kart" konteyneri olusturur (kaydirma kapali).
lv_obj_t *ui_theme_create_card(lv_obj_t *parent);

// Bir etiketi baslik stiliyle bicimlendirir (buyuk, yuksek kontrast).
void ui_theme_style_title(lv_obj_t *label);

// Bir etiketi ikincil/aciklama stiliyle bicimlendirir (kucuk, sonuk renk).
void ui_theme_style_subtitle(lv_obj_t *label);

// Durum noktasi (yesil/turuncu/kirmizi) icin kucuk bir daire olusturur.
lv_obj_t *ui_theme_create_status_dot(lv_obj_t *parent, lv_color_t color);
