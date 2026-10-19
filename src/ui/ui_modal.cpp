#include "ui_modal.h"
#include "ui_theme.h"

static lv_obj_t *s_modal_root = NULL;

static void back_button_event_cb(lv_event_t *e)
{
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    ui_modal_close();
  }
}

lv_obj_t *ui_modal_open(const char *title)
{
  ui_modal_close();

  s_modal_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s_modal_root, UI_SCREEN_W, UI_SCREEN_H);
  lv_obj_set_pos(s_modal_root, 0, 0);
  lv_obj_set_style_radius(s_modal_root, 0, 0);
  lv_obj_set_style_border_width(s_modal_root, 0, 0);
  ui_theme_apply_bg_gradient(s_modal_root);
  lv_obj_clear_flag(s_modal_root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title_label = lv_label_create(s_modal_root);
  lv_label_set_text(title_label, title);
  ui_theme_style_title(title_label);

  lv_obj_t *back_btn = lv_btn_create(s_modal_root);
  lv_obj_set_style_bg_color(back_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(back_btn, back_button_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_obj_center(back_label);

#if UI_COMPACT
  // Kompakt: GERI sol ustte, baslik yaninda; icerik tum alt alani kullanir.
  lv_obj_set_style_pad_all(s_modal_root, 0, 0);
  // lv_layer_top ekranin alt nesnesi degil: yazi tipi burada ayrica verilir.
  lv_obj_set_style_text_font(s_modal_root, UI_FONT_SMALL, 0);
  lv_obj_set_size(back_btn, 76, UI_MODAL_HEADER_HEIGHT - 6);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, UI_SAFE_LEFT, UI_SAFE_TOP);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " GERI");
  lv_obj_set_style_text_font(back_label, UI_FONT_BUTTON, 0);
  // Baslik GERI tusunun hemen saginda: uzun basliklar tusla cakismaz.
  lv_obj_set_width(title_label, UI_SAFE_RIGHT - UI_SAFE_LEFT - 76 - 12);
  lv_label_set_long_mode(title_label, LV_LABEL_LONG_DOT);
  lv_obj_align_to(title_label, back_btn, LV_ALIGN_OUT_RIGHT_MID, 12, 0);
#else
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, UI_SAFE_TOP);
  lv_obj_set_size(back_btn, 90, 44);
  lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -18);
  lv_label_set_text(back_label, "GERI");
#endif

  lv_obj_t *content = lv_obj_create(s_modal_root);
  lv_obj_set_size(content, UI_CONTENT_WIDTH, UI_MODAL_CONTENT_HEIGHT);
#if UI_COMPACT
  lv_obj_set_pos(content, UI_SAFE_LEFT, UI_MODAL_CONTENT_TOP);
#else
  lv_obj_align(content, LV_ALIGN_TOP_MID, 0, UI_MODAL_CONTENT_TOP);
#endif
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_pad_all(content, 4, 0);
  lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(content, 6, 0);

  return content;
}

void ui_modal_close()
{
  if (s_modal_root != NULL) {
    lv_obj_del(s_modal_root);
    s_modal_root = NULL;
  }
}

bool ui_modal_is_open()
{
  return s_modal_root != NULL;
}

lv_obj_t *ui_modal_root()
{
  return s_modal_root;
}
