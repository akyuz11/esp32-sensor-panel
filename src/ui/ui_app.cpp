#include "ui_app.h"
#include "ui_theme.h"
#include "ui_pages.h"
#include "ui_modal.h"
#include "ui_debug.h"

#include <Arduino.h>

#define NAV_COUNT 4

static const char *s_nav_labels[NAV_COUNT] = {"Durum", "Veri", "Menu", "Kayit"};
static lv_obj_t *s_tileview;
static lv_obj_t *s_tiles[NAV_COUNT];
static lv_obj_t *s_nav_buttons[NAV_COUNT];
static lv_obj_t *s_nav_labels_obj[NAV_COUNT];
static int8_t s_active_index = 0;

static void set_nav_highlight(int index)
{
  for (int i = 0; i < NAV_COUNT; i++) {
    bool active = (i == index);
    lv_obj_set_style_bg_color(s_nav_buttons[i], active ? UI_COLOR_ACCENT_FILL : UI_COLOR_CARD, 0);
    lv_obj_set_style_text_color(s_nav_labels_obj[i], active ? lv_color_white() : UI_COLOR_TEXT_DIM, 0);
  }
  s_active_index = index;
}

static void nav_button_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }
  ui_modal_close();
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  lv_obj_set_tile_id(s_tileview, index, 0, LV_ANIM_ON);
  set_nav_highlight(index);
}

static void tileview_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
    return;
  }
  lv_obj_t *active_tile = lv_tileview_get_tile_act(s_tileview);
  for (int i = 0; i < NAV_COUNT; i++) {
    if (s_tiles[i] == active_tile) {
      set_nav_highlight(i);
      break;
    }
  }
}

static void refresh_timer_cb(lv_timer_t *timer)
{
  ui_page_status_update();
  ui_page_data_update();
  ui_page_menu_update();
  ui_page_log_update();
}

static lv_obj_t *create_nav_bar(lv_obj_t *parent)
{
  lv_obj_t *bar = lv_obj_create(parent);
  lv_obj_set_size(bar, UI_NAV_WIDTH, UI_NAV_HEIGHT);
  lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, UI_NAV_TOP);
  // Seffaf: alttaki tileview gradyani nav bar arkasinda da kesintisiz gorunur.
  lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_set_style_pad_all(bar, 4, 0);
  lv_obj_set_style_pad_column(bar, 4, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);

  for (int i = 0; i < NAV_COUNT; i++) {
    lv_obj_t *btn = lv_btn_create(bar);
    lv_obj_set_flex_grow(btn, 1);
    lv_obj_set_height(btn, UI_NAV_HEIGHT - 8);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_bg_color(btn, UI_COLOR_CARD, 0);
    lv_obj_add_event_cb(btn, nav_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, s_nav_labels[i]);
    lv_obj_set_style_text_font(label, UI_FONT_NAV, 0);
    lv_obj_center(label);

    s_nav_buttons[i] = btn;
    s_nav_labels_obj[i] = label;
  }

  return bar;
}

void ui_app_init()
{
#if UI_THEME_MONO
  // LVGL varsayilan temasi acik zemin + mavi ana renk kullanir; acikca renk
  // verilmemis parcalar (slider zemini, odak cercevesi) da gri/koyu olsun.
  lv_disp_t *disp = lv_disp_get_default();
  lv_disp_set_theme(disp, lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_GREY),
                                                lv_palette_darken(LV_PALETTE_GREY, 2), true,
                                                LV_FONT_DEFAULT));
#endif
  lv_obj_t *scr = lv_scr_act();
  ui_theme_apply_screen(scr);

  s_tileview = lv_tileview_create(scr);
  lv_obj_set_size(s_tileview, UI_SCREEN_W, UI_SCREEN_H);
  lv_obj_set_pos(s_tileview, 0, 0);
  ui_theme_apply_bg_gradient(s_tileview);
#if UI_COMPACT
  // Yatay kaydirma cubugu nav barin altinda ince bir cizgi olarak gorunuyordu.
  lv_obj_set_scrollbar_mode(s_tileview, LV_SCROLLBAR_MODE_OFF);
#endif
  lv_obj_add_event_cb(s_tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  s_tiles[0] = lv_tileview_add_tile(s_tileview, 0, 0, LV_DIR_HOR);
  s_tiles[1] = lv_tileview_add_tile(s_tileview, 1, 0, LV_DIR_HOR);
  s_tiles[2] = lv_tileview_add_tile(s_tileview, 2, 0, LV_DIR_HOR);
  s_tiles[3] = lv_tileview_add_tile(s_tileview, 3, 0, LV_DIR_HOR);

  ui_page_status_create(s_tiles[0]);
  ui_page_data_create(s_tiles[1]);
  ui_page_menu_create(s_tiles[2]);
  ui_page_log_create(s_tiles[3]);

  // Nav bar, tileview'in en ustunde, ekrana sabit (tile'lar kaydiginda hareket etmez).
  create_nav_bar(scr);
  set_nav_highlight(0);

  // Ilk deger doldurma + periyodik guncelleme (1 saniye).
  refresh_timer_cb(NULL);
  lv_timer_create(refresh_timer_cb, 1000, NULL);

  ui_debug_init();
}

void ui_app_show_tile(int index)
{
  if (index < 0 || index >= NAV_COUNT) {
    return;
  }
  lv_obj_set_tile_id(s_tileview, index, 0, LV_ANIM_OFF);
  set_nav_highlight(index);
}

uint32_t ui_app_inactivity_ms()
{
  return lv_disp_get_inactive_time(NULL);
}
