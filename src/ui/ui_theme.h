#pragma once
#include <lvgl.h>

// ---------------------------------------------------------------------------
// Yuvarlak 412x412 ekran icin guvenli icerik alani ve ortak koyu tema.
// ---------------------------------------------------------------------------

#define UI_SAFE_LEFT     38
#define UI_SAFE_RIGHT    374
#define UI_SAFE_TOP      25
#define UI_SAFE_BOTTOM   380

// Nav bar: yuvarlak ekranda dis tuslarin koseleri kesilmesin diye tam genislik
// yerine daraltilmis ve ekran altindan yukari alinmis (tus koseleri merkeze ~200px).
#define UI_NAV_HEIGHT    44
#define UI_NAV_WIDTH     248
#define UI_NAV_BOTTOM    372   // nav barin alt kenarinin y koordinati
#define UI_NAV_TOP       (UI_NAV_BOTTOM - UI_NAV_HEIGHT)

// Nav barin ustunde kalan, sayfa icerigi icin kullanilabilir guvenli alan.
#define UI_CONTENT_LEFT    UI_SAFE_LEFT
#define UI_CONTENT_TOP     UI_SAFE_TOP
#define UI_CONTENT_WIDTH   (UI_SAFE_RIGHT - UI_SAFE_LEFT)
#define UI_CONTENT_HEIGHT  (UI_NAV_TOP - UI_SAFE_TOP)

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
