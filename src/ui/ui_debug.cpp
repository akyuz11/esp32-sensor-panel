// Gelistirici araci: seri port uzerinden ekran goruntusu ve uzaktan gezinme.
// Yalnizca -DUI_DEBUG_SCREENSHOT=1 ile derlenir (platformio.ini env'i).
//
// Komutlar (satir sonu \n):
//   SHOT       -> "SHOT <w> <h> <checksum>\n" + w*h*2 bayt RGB565 (LE) + "\nSHOTEND\n"
//   TILE <n>   -> n. sayfaya gec (0..3)
//   TAP <x> <y>-> (x,y) noktasindaki tiklanabilir nesneye CLICKED gonder
//   BACK       -> acik modali kapat
//
// Tum LVGL erisimi bir lv_timer icinde, yani UI gorevinde yapilir.

#include "ui_debug.h"

#if UI_DEBUG_SCREENSHOT

#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>

#include "ui_app.h"
#include "ui_modal.h"

static char s_line[32];
static uint8_t s_line_len = 0;

static void send_screenshot()
{
  // Modal aciksa (lv_layer_top) onu, degilse aktif ekrani yakala.
  lv_obj_t *target = ui_modal_root();
  if (target == NULL) {
    target = lv_scr_act();
  }

  lv_obj_update_layout(target);
  uint32_t size = lv_snapshot_buf_size_needed(target, LV_IMG_CF_TRUE_COLOR);
  uint8_t *buf = (uint8_t *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  if (buf == NULL) {
    Serial.println("SHOTERR bellek");
    return;
  }

  lv_img_dsc_t dsc;
  if (lv_snapshot_take_to_buf(target, LV_IMG_CF_TRUE_COLOR, &dsc, buf, size) != LV_RES_OK) {
    heap_caps_free(buf);
    Serial.println("SHOTERR snapshot");
    return;
  }

  uint32_t n = (uint32_t)dsc.header.w * dsc.header.h * 2;
  uint32_t sum = 0;
  for (uint32_t i = 0; i < n; i++) {
    sum += buf[i];
  }
  Serial.printf("SHOT %u %u %lu\n", (unsigned)dsc.header.w, (unsigned)dsc.header.h, (unsigned long)sum);
  Serial.flush();
  Serial.write(buf, n);
  Serial.flush();
  Serial.print("\nSHOTEND\n");
  heap_caps_free(buf);
}

static void tap(lv_coord_t x, lv_coord_t y)
{
  lv_point_t p = {x, y};
  lv_obj_t *root = ui_modal_root();
  lv_obj_t *obj = lv_indev_search_obj(root ? root : lv_scr_act(), &p);
  while (obj != NULL && !lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) {
    obj = lv_obj_get_parent(obj);
  }
  if (obj == NULL) {
    Serial.println("TAP: nesne yok");
    return;
  }
  lv_event_send(obj, LV_EVENT_PRESSED, NULL);
  lv_event_send(obj, LV_EVENT_RELEASED, NULL);
  lv_event_send(obj, LV_EVENT_CLICKED, NULL);
  lv_disp_trig_activity(NULL);
  Serial.printf("TAP %d %d ok\n", x, y);
}

static void handle_line(const char *line)
{
  int a, b;
  if (strcmp(line, "SHOT") == 0) {
    send_screenshot();
  } else if (sscanf(line, "TILE %d", &a) == 1) {
    ui_modal_close();
    ui_app_show_tile(a);
    Serial.printf("TILE %d ok\n", a);
  } else if (sscanf(line, "TAP %d %d", &a, &b) == 2) {
    tap(a, b);
  } else if (strcmp(line, "BACK") == 0) {
    ui_modal_close();
    Serial.println("BACK ok");
  }
}

static void debug_timer_cb(lv_timer_t *t)
{
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (s_line_len > 0) {
        s_line[s_line_len] = '\0';
        handle_line(s_line);
        s_line_len = 0;
      }
    } else if (s_line_len < sizeof(s_line) - 1) {
      s_line[s_line_len++] = c;
    }
  }
}

void ui_debug_init()
{
  lv_timer_create(debug_timer_cb, 50, NULL);
  Serial.println("[UI] Ekran goruntusu araci acik (SHOT / TILE n / TAP x y / BACK)");
}

#else

void ui_debug_init() {}

#endif
