#include "ui_theme.h"

void ui_theme_apply_bg_gradient(lv_obj_t *obj)
{
  lv_obj_set_style_bg_color(obj, UI_COLOR_BG, 0);
  lv_obj_set_style_bg_grad_color(obj, UI_COLOR_BG_GRAD_END, 0);
  lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_dither_mode(obj, LV_DITHER_ORDERED, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

void ui_theme_apply_screen(lv_obj_t *scr)
{
  ui_theme_apply_bg_gradient(scr);
  lv_obj_set_style_text_color(scr, UI_COLOR_TEXT, 0);
#if UI_COMPACT
  // Yazi tipi kalitimla tum alt nesnelere gecer (lv_conf varsayilani 18 punto).
  lv_obj_set_style_text_font(scr, UI_FONT_SMALL, 0);
#endif
}

lv_obj_t *ui_theme_create_card(lv_obj_t *parent)
{
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_style_bg_color(card, UI_COLOR_CARD, 0);
  lv_obj_set_style_bg_grad_color(card, UI_COLOR_CARD_GRAD_END, 0);
  lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(card, UI_COLOR_CARD_BORDER, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
  lv_obj_set_style_pad_all(card, UI_CARD_PAD, 0);
  // Hafif camgobegi parlama (LV_SHADOW_CACHE_SIZE sayesinde tum kartlar ayni golgeyi paylasir).
  lv_obj_set_style_shadow_width(card, 10, 0);
  lv_obj_set_style_shadow_color(card, UI_COLOR_ACCENT, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

void ui_theme_style_title(lv_obj_t *label)
{
  lv_obj_set_style_text_color(label, UI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(label, UI_FONT_TITLE, 0);
}

void ui_theme_style_subtitle(lv_obj_t *label)
{
  lv_obj_set_style_text_color(label, UI_COLOR_TEXT_DIM, 0);
  lv_obj_set_style_text_font(label, UI_FONT_SMALL, 0);
}

lv_obj_t *ui_theme_create_status_dot(lv_obj_t *parent, lv_color_t color)
{
  lv_obj_t *dot = lv_obj_create(parent);
  lv_obj_set_size(dot, 12, 12);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dot, color, 0);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(dot, 0, 0);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
  return dot;
}
