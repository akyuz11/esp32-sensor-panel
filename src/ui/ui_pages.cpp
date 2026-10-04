#include "ui_pages.h"
#include "ui_theme.h"
#include "ui_modal.h"
#include "app_state.h"
#include "app_config.h"
#include "services/rtc_service.h"
#include "services/wifi_service.h"
#include "services/system_service.h"
#include "services/mqtt_service.h"
#include "services/logging_service.h"
#include "services/weather_service.h"

#include <stdio.h>
#include <esp_random.h>

// Sayfa 4 (KAYIT) buton geri cagrilari; Sayfa 3'teki (MENU) Kayit alt
// ekraninda da ayni davranis kullanildigi icin ileri bildirim gerekiyor.
static void log_start_stop_btn_cb(lv_event_t *e);
static void log_save_now_btn_cb(lv_event_t *e);

// ===========================================================================
// Sayfa 1: DURUM
// ===========================================================================

static lv_obj_t *s_clock_label;
static lv_obj_t *s_date_label;
static lv_obj_t *s_battery_arc;
static lv_obj_t *s_battery_percent_label;
static lv_obj_t *s_battery_voltage_label;

static lv_obj_t *s_wifi_dot, *s_wifi_title, *s_wifi_detail;
static lv_obj_t *s_mqtt_dot, *s_mqtt_title, *s_mqtt_detail;
static lv_obj_t *s_sd_dot, *s_sd_title, *s_sd_detail;
static lv_obj_t *s_system_info_label;

static void open_wifi_detail_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("WI-FI");
  char buf[96];

  lv_obj_t *l1 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "SSID: %s", g_systemStatus.wifiConnected ? g_systemStatus.wifiSsid : "-");
  lv_label_set_text(l1, buf);
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "IP: %s", g_systemStatus.wifiConnected ? g_systemStatus.wifiIp : "-");
  lv_label_set_text(l2, buf);
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "RSSI: %d dBm", g_systemStatus.wifiRssi);
  lv_label_set_text(l3, buf);
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);

  lv_obj_t *l4 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "MAC: %s", g_systemStatus.wifiMac);
  lv_label_set_text(l4, buf);
  ui_theme_style_subtitle(l4);
  lv_obj_set_style_text_color(l4, UI_COLOR_TEXT, 0);
}

static void open_mqtt_detail_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("MQTT");
  char buf[96];

  lv_obj_t *l1 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Sunucu: %s", g_systemStatus.mqttHost);
  lv_label_set_text(l1, buf);
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Port: %u", g_systemStatus.mqttPort);
  lv_label_set_text(l2, buf);
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  lv_label_set_text(l3, g_systemStatus.mqttConnected ? "Durum: baglandi" : "Durum: baglanti yok");
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);

  lv_obj_t *l4 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Son topic: %s", g_systemStatus.mqttLastTopic[0] ? g_systemStatus.mqttLastTopic : "-");
  lv_label_set_text(l4, buf);
  ui_theme_style_subtitle(l4);
  lv_obj_set_style_text_color(l4, UI_COLOR_TEXT, 0);
}

static void open_sd_detail_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("TF KART");
  char buf[96];

  lv_obj_t *l1 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Kapasite: %llu MB", (unsigned long long)(g_systemStatus.sdCapacityBytes / (1024 * 1024)));
  lv_label_set_text(l1, buf);
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Kullanilan: %llu MB", (unsigned long long)(g_systemStatus.sdUsedBytes / (1024 * 1024)));
  lv_label_set_text(l2, buf);
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Aktif dosya: %s", g_loggingStatus.activeFilename[0] ? g_loggingStatus.activeFilename : "-");
  lv_label_set_text(l3, buf);
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);
}

static void open_system_detail_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("SISTEM");
  char buf[64];

  lv_obj_t *l1 = lv_label_create(content);
  uint32_t s = g_systemStatus.uptimeSeconds;
  snprintf(buf, sizeof(buf), "Calisma suresi: %luh %02lum %02lus", (unsigned long)(s / 3600), (unsigned long)((s / 60) % 60), (unsigned long)(s % 60));
  lv_label_set_text(l1, buf);
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Serbest heap: %lu KB", (unsigned long)(g_systemStatus.freeHeap / 1024));
  lv_label_set_text(l2, buf);
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  if (g_systemStatus.psramPresent) {
    snprintf(buf, sizeof(buf), "PSRAM: %lu KB serbest", (unsigned long)(g_systemStatus.freePsram / 1024));
  } else {
    snprintf(buf, sizeof(buf), "PSRAM: bulunamadi");
  }
  lv_label_set_text(l3, buf);
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);
}

static lv_obj_t *create_badge(lv_obj_t *parent, const char *title, lv_obj_t **out_dot, lv_obj_t **out_title, lv_obj_t **out_detail)
{
  lv_obj_t *card = ui_theme_create_card(parent);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(card, 2, 0);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *row = lv_obj_create(card);
  lv_obj_set_size(row, LV_PCT(100), 16);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *dot = ui_theme_create_status_dot(row, UI_COLOR_BAD);
  lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t *title_label = lv_label_create(row);
  lv_label_set_text(title_label, title);
  ui_theme_style_subtitle(title_label);
  lv_obj_set_style_text_color(title_label, UI_COLOR_TEXT, 0);
  lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 18, 0);

  lv_obj_t *detail_label = lv_label_create(card);
  lv_label_set_text(detail_label, "-");
  ui_theme_style_subtitle(detail_label);
  lv_obj_set_width(detail_label, LV_PCT(100));
  lv_label_set_long_mode(detail_label, LV_LABEL_LONG_DOT);

  *out_dot = dot;
  *out_title = title_label;
  *out_detail = detail_label;
  return card;
}

void ui_page_status_create(lv_obj_t *tile)
{
  lv_obj_t *content = lv_obj_create(tile);
  lv_obj_set_size(content, UI_CONTENT_WIDTH, UI_CONTENT_HEIGHT);
  lv_obj_align(content, LV_ALIGN_TOP_MID, 0, UI_CONTENT_TOP);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_pad_all(content, 0, 0);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  // Saat / tarih
  s_clock_label = lv_label_create(content);
  lv_label_set_text(s_clock_label, "--:--");
  lv_obj_set_style_text_font(s_clock_label, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_color(s_clock_label, UI_COLOR_TEXT, 0);
  lv_obj_align(s_clock_label, LV_ALIGN_TOP_MID, 0, 0);

  s_date_label = lv_label_create(content);
  lv_label_set_text(s_date_label, "--.--.----");
  ui_theme_style_subtitle(s_date_label);
  lv_obj_align_to(s_date_label, s_clock_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  // Pil gostergesi (dairesel arc)
  lv_obj_t *bat_box = lv_obj_create(content);
  lv_obj_set_size(bat_box, 140, 118);
  lv_obj_set_style_bg_opa(bat_box, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(bat_box, 0, 0);
  lv_obj_clear_flag(bat_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align_to(bat_box, s_date_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

  s_battery_arc = lv_arc_create(bat_box);
  lv_obj_set_size(s_battery_arc, 110, 110);
  lv_arc_set_range(s_battery_arc, 0, 100);
  lv_arc_set_value(s_battery_arc, 0);
  lv_arc_set_bg_angles(s_battery_arc, 0, 360);
  lv_arc_set_rotation(s_battery_arc, 270);
  lv_obj_remove_style(s_battery_arc, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(s_battery_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(s_battery_arc, UI_COLOR_CARD_BORDER, LV_PART_MAIN);
  lv_obj_set_style_arc_color(s_battery_arc, UI_COLOR_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(s_battery_arc, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_width(s_battery_arc, 10, LV_PART_INDICATOR);
  lv_obj_center(s_battery_arc);

  s_battery_percent_label = lv_label_create(bat_box);
  lv_label_set_text(s_battery_percent_label, "PIL YOK");
  lv_obj_set_style_text_font(s_battery_percent_label, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(s_battery_percent_label, UI_COLOR_TEXT, 0);
  lv_obj_align(s_battery_percent_label, LV_ALIGN_CENTER, 0, -8);

  s_battery_voltage_label = lv_label_create(bat_box);
  lv_label_set_text(s_battery_voltage_label, "");
  ui_theme_style_subtitle(s_battery_voltage_label);
  lv_obj_align(s_battery_voltage_label, LV_ALIGN_CENTER, 0, 16);

  // Durum rozetleri (Wi-Fi / MQTT / SD)
  lv_obj_t *badges_row = lv_obj_create(content);
  lv_obj_set_size(badges_row, LV_PCT(100), 78);
  lv_obj_set_style_bg_opa(badges_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(badges_row, 0, 0);
  lv_obj_set_style_pad_all(badges_row, 0, 0);
  lv_obj_set_style_pad_column(badges_row, 6, 0);
  lv_obj_clear_flag(badges_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(badges_row, LV_FLEX_FLOW_ROW);
  lv_obj_align_to(badges_row, bat_box, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

  lv_obj_t *wifi_card = create_badge(badges_row, "Wi-Fi", &s_wifi_dot, &s_wifi_title, &s_wifi_detail);
  lv_obj_set_size(wifi_card, 104, 74);
  lv_obj_add_event_cb(wifi_card, open_wifi_detail_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *mqtt_card = create_badge(badges_row, "MQTT", &s_mqtt_dot, &s_mqtt_title, &s_mqtt_detail);
  lv_obj_set_size(mqtt_card, 104, 74);
  lv_obj_add_event_cb(mqtt_card, open_mqtt_detail_cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *sd_card = create_badge(badges_row, "SD", &s_sd_dot, &s_sd_title, &s_sd_detail);
  lv_obj_set_size(sd_card, 104, 74);
  lv_obj_add_event_cb(sd_card, open_sd_detail_cb, LV_EVENT_CLICKED, NULL);

  // Kucuk, tiklanabilir sistem bilgisi satiri (calisma suresi/heap/PSRAM
  // ayrintilari kalabalik olmasin diye yalnizca bu ayrinti penceresinde).
  s_system_info_label = lv_label_create(content);
  lv_label_set_text(s_system_info_label, "Sistem >");
  ui_theme_style_subtitle(s_system_info_label);
  lv_obj_align_to(s_system_info_label, badges_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
  lv_obj_add_flag(s_system_info_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(s_system_info_label, open_system_detail_cb, LV_EVENT_CLICKED, NULL);
}

void ui_page_status_update()
{
  // Saat / tarih
  if (g_systemStatus.rtcValid) {
    lv_label_set_text(s_clock_label, rtc_service_time_text());
    lv_label_set_text(s_date_label, rtc_service_date_text());
  } else {
    lv_label_set_text(s_clock_label, "--:--");
    lv_label_set_text(s_date_label, "SAAT AYARLANMADI");
  }

  // Pil
  if (!g_systemStatus.batteryValid && g_systemStatus.batteryVoltage <= 0.01f) {
    lv_label_set_text(s_battery_percent_label, "PIL YOK");
    lv_label_set_text(s_battery_voltage_label, "");
    lv_arc_set_value(s_battery_arc, 0);
  } else if (!g_systemStatus.batteryValid) {
    lv_label_set_text(s_battery_percent_label, "OLCULEMIYOR");
    lv_label_set_text(s_battery_voltage_label, "");
  } else {
    char pbuf[16], vbuf[16];
    snprintf(pbuf, sizeof(pbuf), "%%%d", g_systemStatus.batteryPercent);
    snprintf(vbuf, sizeof(vbuf), "%.2f V", g_systemStatus.batteryVoltage);
    lv_label_set_text(s_battery_percent_label, pbuf);
    lv_label_set_text(s_battery_voltage_label, vbuf);
    lv_arc_set_value(s_battery_arc, g_systemStatus.batteryPercent);

    lv_color_t arc_color = UI_COLOR_ACCENT;
    if (g_systemStatus.batteryPercent <= 20) arc_color = UI_COLOR_BAD;
    else if (g_systemStatus.batteryPercent <= 45) arc_color = UI_COLOR_WARN;
    lv_obj_set_style_arc_color(s_battery_arc, arc_color, LV_PART_INDICATOR);
  }

  // Wi-Fi rozeti
  lv_obj_set_style_bg_color(s_wifi_dot, g_systemStatus.wifiConnected ? UI_COLOR_GOOD : UI_COLOR_BAD, 0);
  if (g_systemStatus.wifiConnected) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%s\n%d dBm", g_systemStatus.wifiSsid, g_systemStatus.wifiRssi);
    lv_label_set_text(s_wifi_detail, buf);
  } else {
    lv_label_set_text(s_wifi_detail, "Bagli degil");
  }

  // MQTT rozeti
  lv_obj_set_style_bg_color(s_mqtt_dot, g_systemStatus.mqttConnected ? UI_COLOR_GOOD : UI_COLOR_WARN, 0);
  lv_label_set_text(s_mqtt_detail,
                    g_systemStatus.mqttConnected
                        ? (g_systemStatus.bleConnected ? "MQTT bagli\nTelefon bagli" : "MQTT bagli")
                        : (g_systemStatus.bleConnected ? "MQTT kapali\nTelefon bagli" : "MQTT kapali"));

  // SD rozeti
  lv_obj_set_style_bg_color(s_sd_dot, g_systemStatus.sdMounted ? UI_COLOR_GOOD : UI_COLOR_BAD, 0);
  lv_label_set_text(s_sd_detail, g_systemStatus.sdMounted ? "SD hazir" : "SD yok");
}

// ===========================================================================
// Sayfa 2: VERILER
// ===========================================================================

struct SensorCardRefs {
  lv_obj_t *value_label;
  lv_obj_t *status_label;
};

static lv_obj_t *s_data_meta_label;
static lv_obj_t *s_data_demo_label;
static SensorCardRefs s_temp_card, s_humidity_card, s_voltage_card, s_power_card;

static lv_obj_t *create_sensor_card(lv_obj_t *parent, const char *name, const char *unit, SensorCardRefs *refs)
{
  lv_obj_t *card = ui_theme_create_card(parent);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

  lv_obj_t *name_label = lv_label_create(card);
  lv_label_set_text(name_label, name);
  ui_theme_style_subtitle(name_label);
  lv_obj_set_style_text_color(name_label, UI_COLOR_TEXT, 0);

  lv_obj_t *value_row = lv_obj_create(card);
  lv_obj_set_size(value_row, LV_PCT(100), 34);
  lv_obj_set_style_bg_opa(value_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(value_row, 0, 0);
  lv_obj_set_style_pad_all(value_row, 0, 0);
  lv_obj_clear_flag(value_row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *value_label = lv_label_create(value_row);
  lv_label_set_text(value_label, "--");
  lv_obj_set_style_text_font(value_label, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(value_label, UI_COLOR_TEXT, 0);
  lv_obj_align(value_label, LV_ALIGN_LEFT_MID, 0, 0);

  lv_obj_t *unit_label = lv_label_create(value_row);
  lv_label_set_text(unit_label, unit);
  ui_theme_style_subtitle(unit_label);
  lv_obj_align_to(unit_label, value_label, LV_ALIGN_OUT_RIGHT_MID, 4, 4);

  lv_obj_t *status_label = lv_label_create(card);
  lv_label_set_text(status_label, "");
  ui_theme_style_subtitle(status_label);
  lv_obj_set_style_text_color(status_label, UI_COLOR_WARN, 0);

  refs->value_label = value_label;
  refs->status_label = status_label;
  return card;
}

void ui_page_data_create(lv_obj_t *tile)
{
  lv_obj_t *content = lv_obj_create(tile);
  lv_obj_set_size(content, UI_CONTENT_WIDTH, UI_CONTENT_HEIGHT);
  lv_obj_align(content, LV_ALIGN_TOP_MID, 0, UI_CONTENT_TOP);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_pad_all(content, 0, 0);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(content);
  lv_label_set_text(title, "CANLI VERILER");
  ui_theme_style_title(title);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_26, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

  s_data_meta_label = lv_label_create(content);
  lv_label_set_text(s_data_meta_label, "Veri yok");
  ui_theme_style_subtitle(s_data_meta_label);
  lv_obj_align_to(s_data_meta_label, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  s_data_demo_label = lv_label_create(content);
  lv_label_set_text(s_data_demo_label, "DEMO");
  lv_obj_set_style_text_color(s_data_demo_label, UI_COLOR_WARN, 0);
  lv_obj_set_style_text_font(s_data_demo_label, &lv_font_montserrat_18, 0);
  lv_obj_align_to(s_data_demo_label, s_data_meta_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);

  lv_obj_t *grid = lv_obj_create(content);
  lv_obj_set_size(grid, LV_PCT(100), 190);
  lv_obj_align_to(grid, s_data_demo_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);
  lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_style_pad_all(grid, 0, 0);
  lv_obj_set_style_pad_gap(grid, 6, 0);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

  static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

  lv_obj_t *c;
  c = create_sensor_card(grid, "Sicaklik", "C", &s_temp_card);
  lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  c = create_sensor_card(grid, "Nem", "%", &s_humidity_card);
  lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  c = create_sensor_card(grid, "Gerilim", "V", &s_voltage_card);
  lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

  c = create_sensor_card(grid, "Guc", "W", &s_power_card);
  lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
}

static void update_sensor_card(SensorCardRefs *refs, bool valid, float value, uint32_t lastUpdateMs, const char *fmt)
{
  if (!valid) {
    lv_label_set_text(refs->value_label, "--");
    lv_label_set_text(refs->status_label, lastUpdateMs == 0 ? "VERI YOK" : "");
    lv_obj_set_style_text_color(refs->value_label, UI_COLOR_TEXT_DIM, 0);
    return;
  }

  char buf[24];
  snprintf(buf, sizeof(buf), fmt, value);
  lv_label_set_text(refs->value_label, buf);

  uint32_t age_s = (millis() - lastUpdateMs) / 1000;
  if (age_s < APP_DATA_FRESH_SECONDS) {
    lv_label_set_text(refs->status_label, "");
    lv_obj_set_style_text_color(refs->value_label, UI_COLOR_TEXT, 0);
  } else if (age_s < APP_DATA_STALE_SECONDS) {
    lv_label_set_text(refs->status_label, "");
    lv_obj_set_style_text_color(refs->value_label, UI_COLOR_WARN, 0);
  } else {
    lv_label_set_text(refs->status_label, "ESKI VERI");
    lv_obj_set_style_text_color(refs->value_label, UI_COLOR_WARN, 0);
  }
}

void ui_page_data_update()
{
  lv_label_set_text(s_data_meta_label, g_systemStatus.mqttConnected ? "MQTT bagli" : "MQTT kapali");
  if (g_sensorValues.demoMode) {
    lv_obj_clear_flag(s_data_demo_label, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(s_data_demo_label, LV_OBJ_FLAG_HIDDEN);
  }

  update_sensor_card(&s_temp_card, g_sensorValues.temperatureValid, g_sensorValues.temperature, g_sensorValues.lastTemperatureUpdateMs, "%.1f");
  update_sensor_card(&s_humidity_card, g_sensorValues.humidityValid, g_sensorValues.humidity, g_sensorValues.lastHumidityUpdateMs, "%.0f");
  update_sensor_card(&s_voltage_card, g_sensorValues.voltageValid, g_sensorValues.voltage, g_sensorValues.lastVoltageUpdateMs, "%.1f");
  update_sensor_card(&s_power_card, g_sensorValues.powerValid, g_sensorValues.power, g_sensorValues.lastPowerUpdateMs, "%.0f");
}

// ===========================================================================
// Sayfa 3: MENU
// ===========================================================================

static void wifi_reconnect_btn_cb(lv_event_t *e)
{
  wifi_service_request_reconnect();
  Serial.println("[UI] Wi-Fi yeniden baglanma istendi");
}

static void wifi_scan_btn_cb(lv_event_t *e)
{
  wifi_service_request_scan();
  Serial.println("[UI] Wi-Fi ag taramasi istendi");
}

static void open_wifi_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("WI-FI");
  char buf[96];

  lv_obj_t *l1 = lv_label_create(content);
  lv_label_set_text(l1, g_systemStatus.wifiConnected ? "Durum: baglandi" : "Durum: baglanti yok");
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "SSID: %s", g_systemStatus.wifiConnected ? g_systemStatus.wifiSsid : "-");
  lv_label_set_text(l2, buf);
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "IP: %s   RSSI: %d dBm", g_systemStatus.wifiConnected ? g_systemStatus.wifiIp : "-", g_systemStatus.wifiRssi);
  lv_label_set_text(l3, buf);
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);

  lv_obj_t *l4 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Yeniden baglanma sayisi: %lu", (unsigned long)g_systemStatus.wifiReconnectCount);
  lv_label_set_text(l4, buf);
  ui_theme_style_subtitle(l4);
  lv_obj_set_style_text_color(l4, UI_COLOR_TEXT, 0);

  lv_obj_t *btn_row = lv_obj_create(content);
  lv_obj_set_size(btn_row, LV_PCT(100), 44);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_set_style_pad_all(btn_row, 0, 0);
  lv_obj_set_style_pad_column(btn_row, 6, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);

  lv_obj_t *reconnect_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(reconnect_btn, 1);
  lv_obj_set_height(reconnect_btn, 44);
  lv_obj_set_style_bg_color(reconnect_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(reconnect_btn, wifi_reconnect_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *reconnect_label = lv_label_create(reconnect_btn);
  lv_label_set_text(reconnect_label, "YENIDEN BAGLAN");
  lv_obj_set_style_text_font(reconnect_label, &lv_font_montserrat_16, 0);
  lv_obj_center(reconnect_label);

  lv_obj_t *scan_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(scan_btn, 1);
  lv_obj_set_height(scan_btn, 44);
  lv_obj_set_style_bg_color(scan_btn, UI_COLOR_CARD_BORDER, 0);
  lv_obj_add_event_cb(scan_btn, wifi_scan_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *scan_label = lv_label_create(scan_btn);
  lv_label_set_text(scan_label, "AGLARI TARA");
  lv_obj_set_style_text_font(scan_label, &lv_font_montserrat_16, 0);
  lv_obj_center(scan_label);
}

static void mqtt_connect_btn_cb(lv_event_t *e)
{
  mqtt_service_request_connect();
  Serial.println("[UI] MQTT baglanma istendi");
}

static void mqtt_disconnect_btn_cb(lv_event_t *e)
{
  mqtt_service_request_disconnect();
  Serial.println("[UI] MQTT baglanti kesme istendi");
}

static void open_mqtt_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("MQTT");
  char buf[96];

  lv_obj_t *l1 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Sunucu: %s:%u", g_systemStatus.mqttHost, g_systemStatus.mqttPort);
  lv_label_set_text(l1, buf);
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Client ID: %s", g_systemStatus.mqttClientId);
  lv_label_set_text(l2, buf);
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  lv_label_set_text(l3, g_systemStatus.mqttConnected ? "Durum: baglandi" : "Durum: baglanti yok");
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);

  lv_obj_t *l4 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Son topic: %s", g_systemStatus.mqttLastTopic[0] ? g_systemStatus.mqttLastTopic : "-");
  lv_label_set_text(l4, buf);
  ui_theme_style_subtitle(l4);
  lv_obj_set_style_text_color(l4, UI_COLOR_TEXT, 0);

  lv_obj_t *l5 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Yeniden baglanma: %lu", (unsigned long)g_systemStatus.mqttReconnectCount);
  lv_label_set_text(l5, buf);
  ui_theme_style_subtitle(l5);
  lv_obj_set_style_text_color(l5, UI_COLOR_TEXT, 0);

  lv_obj_t *btn_row = lv_obj_create(content);
  lv_obj_set_size(btn_row, LV_PCT(100), 40);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_set_style_pad_all(btn_row, 0, 0);
  lv_obj_set_style_pad_column(btn_row, 4, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);

  lv_obj_t *connect_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(connect_btn, 1);
  lv_obj_set_height(connect_btn, 40);
  lv_obj_set_style_bg_color(connect_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(connect_btn, mqtt_connect_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *connect_label = lv_label_create(connect_btn);
  lv_label_set_text(connect_label, "BAGLAN");
  lv_obj_set_style_text_font(connect_label, &lv_font_montserrat_16, 0);
  lv_obj_center(connect_label);

  lv_obj_t *disconnect_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(disconnect_btn, 1);
  lv_obj_set_height(disconnect_btn, 40);
  lv_obj_set_style_bg_color(disconnect_btn, UI_COLOR_CARD_BORDER, 0);
  lv_obj_add_event_cb(disconnect_btn, mqtt_disconnect_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *disconnect_label = lv_label_create(disconnect_btn);
  lv_label_set_text(disconnect_label, "KES");
  lv_obj_set_style_text_font(disconnect_label, &lv_font_montserrat_16, 0);
  lv_obj_center(disconnect_label);

  lv_obj_t *retry_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(retry_btn, 1);
  lv_obj_set_height(retry_btn, 40);
  lv_obj_set_style_bg_color(retry_btn, UI_COLOR_CARD_BORDER, 0);
  lv_obj_add_event_cb(retry_btn, mqtt_connect_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *retry_label = lv_label_create(retry_btn);
  lv_label_set_text(retry_label, "TEKRAR DENE");
  lv_obj_set_style_text_font(retry_label, &lv_font_montserrat_16, 0);
  lv_obj_center(retry_label);
}

static void open_logging_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("KAYIT");
  char buf[96];

  lv_obj_t *l1 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Durum: %s", g_loggingStatus.enabled ? "acik" : "kapali");
  lv_label_set_text(l1, buf);
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Kayit araligi: %u sn", g_loggingStatus.intervalSeconds);
  lv_label_set_text(l2, buf);
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Aktif dosya: %s", g_loggingStatus.activeFilename[0] ? g_loggingStatus.activeFilename : "-");
  lv_label_set_text(l3, buf);
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);

  lv_obj_t *l4 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "Toplam satir: %lu", (unsigned long)g_loggingStatus.rowCount);
  lv_label_set_text(l4, buf);
  ui_theme_style_subtitle(l4);
  lv_obj_set_style_text_color(l4, UI_COLOR_TEXT, 0);

  lv_obj_t *l5 = lv_label_create(content);
  if (g_loggingStatus.lastWriteMs == 0) {
    lv_label_set_text(l5, "Son kayit: -");
  } else {
    snprintf(buf, sizeof(buf), "Son kayit: %lu sn once", (unsigned long)((millis() - g_loggingStatus.lastWriteMs) / 1000));
    lv_label_set_text(l5, buf);
  }
  ui_theme_style_subtitle(l5);
  lv_obj_set_style_text_color(l5, UI_COLOR_TEXT, 0);

  lv_obj_t *btn_row = lv_obj_create(content);
  lv_obj_set_size(btn_row, LV_PCT(100), 40);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_set_style_pad_all(btn_row, 0, 0);
  lv_obj_set_style_pad_column(btn_row, 4, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);

  lv_obj_t *toggle_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(toggle_btn, 1);
  lv_obj_set_height(toggle_btn, 40);
  lv_obj_set_style_bg_color(toggle_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(toggle_btn, log_start_stop_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *toggle_label = lv_label_create(toggle_btn);
  lv_label_set_text(toggle_label, g_loggingStatus.enabled ? "KAYDI DURDUR" : "KAYDI BASLAT");
  lv_obj_set_style_text_font(toggle_label, &lv_font_montserrat_16, 0);
  lv_obj_center(toggle_label);

  lv_obj_t *test_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(test_btn, 1);
  lv_obj_set_height(test_btn, 40);
  lv_obj_set_style_bg_color(test_btn, UI_COLOR_CARD_BORDER, 0);
  lv_obj_add_event_cb(test_btn, log_save_now_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *test_label = lv_label_create(test_btn);
  lv_label_set_text(test_label, "TEST SATIRI YAZ");
  lv_obj_set_style_text_font(test_label, &lv_font_montserrat_16, 0);
  lv_obj_center(test_label);
}

static void demo_toggle_btn_cb(lv_event_t *e)
{
  g_sensorValues.demoMode = !g_sensorValues.demoMode;
  Serial.printf("[UI] Demo modu: %s\r\n", g_sensorValues.demoMode ? "acik" : "kapali");
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  lv_label_set_text(label, g_sensorValues.demoMode ? "DEMO: ACIK" : "DEMO: KAPALI");
}

static void open_sensors_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("SENSORLER");
  lv_obj_t *l = lv_label_create(content);
  lv_label_set_text(l,
    "Sicaklik: " MQTT_TOPIC_TEMPERATURE "\n"
    "Nem: " MQTT_TOPIC_HUMIDITY "\n"
    "Gerilim: " MQTT_TOPIC_VOLTAGE "\n"
    "Akim: " MQTT_TOPIC_CURRENT "\n"
    "Guc: " MQTT_TOPIC_POWER);
  ui_theme_style_subtitle(l);
  lv_obj_set_style_text_color(l, UI_COLOR_TEXT, 0);
  lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(l, LV_PCT(100));

  lv_obj_t *demo_btn = lv_btn_create(content);
  lv_obj_set_size(demo_btn, LV_PCT(100), 44);
  lv_obj_set_style_bg_color(demo_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_t *demo_label = lv_label_create(demo_btn);
  lv_label_set_text(demo_label, g_sensorValues.demoMode ? "DEMO: ACIK" : "DEMO: KAPALI");
  lv_obj_center(demo_label);
  lv_obj_add_event_cb(demo_btn, demo_toggle_btn_cb, LV_EVENT_CLICKED, demo_label);
}

static void ntp_sync_btn_cb(lv_event_t *e)
{
  rtc_service_request_ntp_sync();
  Serial.println("[UI] NTP senkronizasyonu istendi");
}

static void write_rtc_btn_cb(lv_event_t *e)
{
  rtc_service_request_write_to_rtc();
  Serial.println("[UI] RTC'ye yazma istendi");
}

static void open_clock_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("SAAT");
  char buf[80];

  lv_obj_t *l1 = lv_label_create(content);
  snprintf(buf, sizeof(buf), "RTC: %s", g_systemStatus.rtcValid ? "gecerli" : "gecerli degil");
  lv_label_set_text(l1, buf);
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *l2 = lv_label_create(content);
  lv_label_set_text(l2, rtc_service_ntp_ever_synced() ? "NTP: bu oturumda esitlendi" : "NTP: henuz esitlenmedi");
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *l3 = lv_label_create(content);
  if (rtc_service_last_ntp_sync_ms() == 0) {
    lv_label_set_text(l3, "Son esitleme: -");
  } else {
    snprintf(buf, sizeof(buf), "Son esitleme: %lu sn once", (unsigned long)((millis() - rtc_service_last_ntp_sync_ms()) / 1000));
    lv_label_set_text(l3, buf);
  }
  ui_theme_style_subtitle(l3);
  lv_obj_set_style_text_color(l3, UI_COLOR_TEXT, 0);

  lv_obj_t *btn_row = lv_obj_create(content);
  lv_obj_set_size(btn_row, LV_PCT(100), 44);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_set_style_pad_all(btn_row, 0, 0);
  lv_obj_set_style_pad_column(btn_row, 6, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);

  lv_obj_t *ntp_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(ntp_btn, 1);
  lv_obj_set_height(ntp_btn, 44);
  lv_obj_set_style_bg_color(ntp_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(ntp_btn, ntp_sync_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *ntp_label = lv_label_create(ntp_btn);
  lv_label_set_text(ntp_label, "NTP ILE ESITLE");
  lv_obj_set_style_text_font(ntp_label, &lv_font_montserrat_16, 0);
  lv_obj_center(ntp_label);

  lv_obj_t *write_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(write_btn, 1);
  lv_obj_set_height(write_btn, 44);
  lv_obj_set_style_bg_color(write_btn, UI_COLOR_CARD_BORDER, 0);
  lv_obj_add_event_cb(write_btn, write_rtc_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *write_label = lv_label_create(write_btn);
  lv_label_set_text(write_label, "RTC'YE YAZ");
  lv_obj_set_style_text_font(write_label, &lv_font_montserrat_16, 0);
  lv_obj_center(write_label);
}

static void brightness_slider_cb(lv_event_t *e)
{
  lv_obj_t *slider = lv_event_get_target(e);
  system_service_set_brightness((uint8_t)lv_slider_get_value(slider));
}

static void sleep_timeout_btn_cb(lv_event_t *e)
{
  uint32_t timeout_ms = (uint32_t)(intptr_t)lv_event_get_user_data(e);
  system_service_set_sleep_timeout_ms(timeout_ms);
  Serial.printf("[UI] Ekran uyku suresi: %lu ms\r\n", (unsigned long)timeout_ms);
}

static void open_display_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("EKRAN");

  lv_obj_t *l1 = lv_label_create(content);
  lv_label_set_text(l1, "Parlaklik");
  ui_theme_style_subtitle(l1);
  lv_obj_set_style_text_color(l1, UI_COLOR_TEXT, 0);

  lv_obj_t *slider = lv_slider_create(content);
  lv_obj_set_size(slider, LV_PCT(100), 16);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, system_service_get_brightness(), LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider, UI_COLOR_ACCENT, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, UI_COLOR_ACCENT, LV_PART_KNOB);
  lv_obj_add_event_cb(slider, brightness_slider_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lv_obj_t *l2 = lv_label_create(content);
  lv_label_set_text(l2, "Ekran uyku suresi");
  ui_theme_style_subtitle(l2);
  lv_obj_set_style_text_color(l2, UI_COLOR_TEXT, 0);

  lv_obj_t *btn_row = lv_obj_create(content);
  lv_obj_set_size(btn_row, LV_PCT(100), 40);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_set_style_pad_all(btn_row, 0, 0);
  lv_obj_set_style_pad_column(btn_row, 4, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);

  struct { const char *label; uint32_t ms; } options[] = {
      {"Kapali", APP_SLEEP_TIMEOUT_OFF},
      {"30sn", APP_SLEEP_TIMEOUT_30S},
      {"60sn", APP_SLEEP_TIMEOUT_60S},
      {"5dk", APP_SLEEP_TIMEOUT_5MIN},
  };
  for (auto &opt : options) {
    lv_obj_t *b = lv_btn_create(btn_row);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_height(b, 40);
    lv_obj_set_style_bg_color(b, system_service_get_sleep_timeout_ms() == opt.ms ? UI_COLOR_ACCENT_FILL : UI_COLOR_CARD_BORDER, 0);
    lv_obj_add_event_cb(b, sleep_timeout_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)opt.ms);
    lv_obj_t *label = lv_label_create(b);
    lv_label_set_text(label, opt.label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_center(label);
  }
}

static void weather_refresh_btn_cb(lv_event_t *e)
{
  weather_service_request_refresh();
  Serial.println("[UI] Hava durumu yenileme istendi");
}

static void open_weather_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("HAVA DURUMU");
  char buf[64];

  lv_obj_t *city = lv_label_create(content);
  lv_label_set_text(city, APP_WEATHER_CITY_NAME);
  ui_theme_style_subtitle(city);
  lv_obj_set_style_text_color(city, UI_COLOR_TEXT, 0);

  lv_obj_t *temp = lv_label_create(content);
  if (g_weatherStatus.valid) {
    snprintf(buf, sizeof(buf), "%.1f C", g_weatherStatus.temperature);
  } else {
    snprintf(buf, sizeof(buf), "-- C");
  }
  lv_label_set_text(temp, buf);
  lv_obj_set_style_text_font(temp, &lv_font_montserrat_26, 0);
  lv_obj_set_style_text_color(temp, UI_COLOR_TEXT, 0);

  lv_obj_t *cond = lv_label_create(content);
  lv_label_set_text(cond, g_weatherStatus.valid ? weather_service_code_text(g_weatherStatus.weatherCode) : "Henuz veri yok");
  ui_theme_style_subtitle(cond);
  lv_obj_set_style_text_color(cond, UI_COLOR_TEXT, 0);

  lv_obj_t *wind = lv_label_create(content);
  if (g_weatherStatus.valid) {
    snprintf(buf, sizeof(buf), "Ruzgar: %.1f km/s", g_weatherStatus.windSpeedKmh);
    lv_label_set_text(wind, buf);
  } else if (!g_systemStatus.wifiConnected) {
    lv_label_set_text(wind, "Wi-Fi bagli degil");
  } else if (g_weatherStatus.lastFetchFailed) {
    lv_label_set_text(wind, "Son deneme basarisiz");
  } else {
    lv_label_set_text(wind, "Bekleniyor...");
  }
  ui_theme_style_subtitle(wind);
  lv_obj_set_style_text_color(wind, UI_COLOR_TEXT, 0);

  lv_obj_t *updated = lv_label_create(content);
  if (g_weatherStatus.lastUpdateMs == 0) {
    lv_label_set_text(updated, "Son guncelleme: -");
  } else {
    snprintf(buf, sizeof(buf), "Son guncelleme: %lu sn once", (unsigned long)((millis() - g_weatherStatus.lastUpdateMs) / 1000));
    lv_label_set_text(updated, buf);
  }
  ui_theme_style_subtitle(updated);
  lv_obj_set_style_text_color(updated, UI_COLOR_TEXT, 0);

  lv_obj_t *refresh_btn = lv_btn_create(content);
  lv_obj_set_size(refresh_btn, LV_PCT(100), 44);
  lv_obj_set_style_bg_color(refresh_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(refresh_btn, weather_refresh_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *refresh_label = lv_label_create(refresh_btn);
  lv_label_set_text(refresh_label, "YENILE");
  lv_obj_center(refresh_label);
}

// --- Salla Kazan -----------------------------------------------------------
// Sallama algilama Driver_Loop'taki motion_service'te yapilir; bu ekran yalnizca
// g_motionStatus.shakeCount'u izler. Animasyon LVGL task'inda bir lv_timer ile
// calisir ve modal kapaninca (label silinince) timer da silinir.

static lv_timer_t *s_lucky_timer = NULL;
static lv_obj_t *s_lucky_hint_label;
static lv_obj_t *s_lucky_number_label;
static lv_obj_t *s_lucky_result_label;
static uint32_t s_lucky_seen_shake;
static bool s_lucky_rolling;
static uint32_t s_lucky_roll_start_ms;
static uint32_t s_lucky_next_step_ms;

static int lucky_random()
{
  return APP_LUCKY_MIN + (int)(esp_random() % (APP_LUCKY_MAX - APP_LUCKY_MIN + 1));
}

static void lucky_timer_cb(lv_timer_t *timer)
{
  uint32_t now = millis();
  char buf[32];

  if (!s_lucky_rolling) {
    if (g_motionStatus.shakeCount == s_lucky_seen_shake) {
      return;
    }
    s_lucky_seen_shake = g_motionStatus.shakeCount;
    s_lucky_rolling = true;
    s_lucky_roll_start_ms = now;
    s_lucky_next_step_ms = now;
    lv_disp_trig_activity(NULL); // sallamak da etkinliktir: ekran uyumasin / uyansin
    lv_label_set_text(s_lucky_hint_label, "Seciliyor...");
    lv_label_set_text(s_lucky_result_label, "");
    lv_obj_set_style_text_color(s_lucky_number_label, UI_COLOR_TEXT_DIM, 0);
  }

  uint32_t elapsed = now - s_lucky_roll_start_ms;
  if (elapsed < APP_LUCKY_ROLL_MS) {
    if (now >= s_lucky_next_step_ms) {
      snprintf(buf, sizeof(buf), "%d", lucky_random());
      lv_label_set_text(s_lucky_number_label, buf);
      // Sona dogru yavaslayan "carkifelek" etkisi: 50 ms'den ~300 ms'ye.
      s_lucky_next_step_ms = now + 50 + (250 * elapsed) / APP_LUCKY_ROLL_MS;
    }
    return;
  }

  s_lucky_rolling = false;
  // Donme sirasinda gelen sallamalar yeni bir tur baslatmasin.
  s_lucky_seen_shake = g_motionStatus.shakeCount;
  lv_disp_trig_activity(NULL);

  int lucky = lucky_random();
  snprintf(buf, sizeof(buf), "%d", lucky);
  lv_label_set_text(s_lucky_number_label, buf);
  lv_obj_set_style_text_color(s_lucky_number_label, UI_COLOR_ACCENT, 0);
  lv_label_set_text(s_lucky_hint_label, "Sansli sayiniz");
  lv_label_set_text(s_lucky_result_label, "Tekrar icin salla");
  Serial.println("[UI] Salla Kazan: sansli sayi = " + String(lucky));
}

static void lucky_screen_deleted_cb(lv_event_t *e)
{
  if (s_lucky_timer != NULL) {
    lv_timer_del(s_lucky_timer);
    s_lucky_timer = NULL;
  }
}

static void open_lucky_menu_cb(lv_event_t *e)
{
  lv_obj_t *content = ui_modal_open("SALLA KAZAN");
  lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(content, 10, 0);

  s_lucky_hint_label = lv_label_create(content);
  lv_label_set_text(s_lucky_hint_label, "Cihazi salla,\nsansli sayini sec!");
  lv_obj_set_style_text_align(s_lucky_hint_label, LV_TEXT_ALIGN_CENTER, 0);
  ui_theme_style_subtitle(s_lucky_hint_label);
  lv_obj_set_style_text_font(s_lucky_hint_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(s_lucky_hint_label, UI_COLOR_TEXT, 0);

  s_lucky_number_label = lv_label_create(content);
  lv_label_set_text(s_lucky_number_label, "?");
  lv_obj_set_style_text_font(s_lucky_number_label, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(s_lucky_number_label, UI_COLOR_ACCENT, 0);

  s_lucky_result_label = lv_label_create(content);
  lv_label_set_text(s_lucky_result_label, "");
  ui_theme_style_subtitle(s_lucky_result_label);

  s_lucky_seen_shake = g_motionStatus.shakeCount; // acilistan onceki sallamalar sayilmaz
  s_lucky_rolling = false;
  if (s_lucky_timer != NULL) {
    lv_timer_del(s_lucky_timer);
  }
  s_lucky_timer = lv_timer_create(lucky_timer_cb, 40, NULL);
  lv_obj_add_event_cb(s_lucky_number_label, lucky_screen_deleted_cb, LV_EVENT_DELETE, NULL);
}

static lv_obj_t *create_menu_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb)
{
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_style_bg_color(btn, UI_COLOR_CARD, 0);
  lv_obj_set_style_border_color(btn, UI_COLOR_CARD_BORDER, 0);
  lv_obj_set_style_border_width(btn, 1, 0);
  lv_obj_set_style_radius(btn, 12, 0);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, UI_COLOR_TEXT, 0);
  lv_obj_center(label);
  return btn;
}

void ui_page_menu_create(lv_obj_t *tile)
{
  lv_obj_t *content = lv_obj_create(tile);
  lv_obj_set_size(content, UI_CONTENT_WIDTH, UI_CONTENT_HEIGHT);
  lv_obj_align(content, LV_ALIGN_TOP_MID, 0, UI_CONTENT_TOP);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_pad_all(content, 0, 0);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(content);
  lv_label_set_text(title, "ISLEMLER");
  ui_theme_style_title(title);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_26, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t *grid = lv_obj_create(content);
  lv_obj_set_size(grid, LV_PCT(100), UI_CONTENT_HEIGHT - 30);
  lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_style_pad_all(grid, 0, 0);
  lv_obj_set_style_pad_gap(grid, 8, 0);
  lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

  static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);

  lv_obj_t *b;
  b = create_menu_button(grid, "Wi-Fi", open_wifi_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  b = create_menu_button(grid, "MQTT", open_mqtt_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
  b = create_menu_button(grid, "Kayit", open_logging_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  b = create_menu_button(grid, "Sensorler", open_sensors_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  b = create_menu_button(grid, "Saat", open_clock_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  b = create_menu_button(grid, "Ekran", open_display_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  b = create_menu_button(grid, "Hava Durumu", open_weather_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 3, 1);
  b = create_menu_button(grid, "Salla Kazan", open_lucky_menu_cb);
  lv_obj_set_grid_cell(b, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 3, 1);
}

void ui_page_menu_update()
{
  // Asama 2'de menu sayfasinin ana gorunumunde canlandirilan bir eleman yok;
  // acik olan alt ekranlar acildiklari anda guncel veriyle doldurulur.
}

// ===========================================================================
// Sayfa 4: KAYIT
// ===========================================================================

static lv_obj_t *s_log_sd_label;
static lv_obj_t *s_log_active_label;
static lv_obj_t *s_log_file_label;
static lv_obj_t *s_log_rows_label;
static lv_obj_t *s_log_last_write_label;
static lv_obj_t *s_log_list;
static lv_obj_t *s_log_start_stop_btn;
static lv_obj_t *s_log_start_stop_label;

static void log_start_stop_btn_cb(lv_event_t *e)
{
  if (g_loggingStatus.enabled) {
    logging_service_stop();
  } else {
    logging_service_start();
  }
}

static void log_save_now_btn_cb(lv_event_t *e)
{
  logging_service_request_write_now();
  Serial.println("[UI] Simdi kaydet istendi");
}

// ---------------------------------------------------------------------------
// "KAYDI IZLE" modali: SD'deki kayitlar, en yeni zaman damgasi en ustte.
// SD okumasi Driver_Loop'ta yapilir; burada yalnizca g_logView okunur.
// ---------------------------------------------------------------------------

static lv_obj_t *s_log_view_list = NULL;     // modal acikken icerik konteyneri
static uint32_t s_log_view_rendered_version = 0;
static uint32_t s_log_view_seen_row_count = 0;
static bool s_log_view_waiting = false;      // yukleme istendi, sonuc bekleniyor

static void log_view_request_reload()
{
  s_log_view_waiting = true;
  s_log_view_seen_row_count = g_loggingStatus.rowCount;
  logging_service_request_view_reload();
}

static void log_view_deleted_cb(lv_event_t *e)
{
  s_log_view_list = NULL; // GERI veya baska bir modal acilinca
}

static void format_log_value(char *out, size_t out_len, bool valid, float value, const char *fmt)
{
  if (valid) {
    snprintf(out, out_len, fmt, value);
  } else {
    snprintf(out, out_len, "-");
  }
}

static void log_view_render()
{
  lv_obj_clean(s_log_view_list);

  lv_obj_t *header = lv_label_create(s_log_view_list);
  ui_theme_style_subtitle(header);
  lv_obj_set_width(header, LV_PCT(100));
  lv_label_set_long_mode(header, LV_LABEL_LONG_WRAP);
  if (g_logView.rowCount == 0) {
    lv_label_set_text(header, g_logView.error[0] ? g_logView.error : "Kayit yok");
    return;
  }
  // "/logs/2026-09-24.csv" -> "2026-09-24"
  const char *day = g_logView.filename;
  if (strncmp(day, "/logs/", 6) == 0) {
    day += 6;
  }
  char buf[96];
  snprintf(buf, sizeof(buf), "%.10s - son %u kayit\nen yeni en ustte", day, g_logView.rowCount);
  lv_label_set_text(header, buf);

  for (uint8_t i = 0; i < g_logView.rowCount; i++) {
    const LogViewRow &r = g_logView.rows[i];
    char temp[12], hum[12], volt[12], cur[12], pwr[12];
    format_log_value(temp, sizeof(temp), r.temperatureValid, r.temperature, "%.1f C");
    format_log_value(hum, sizeof(hum), r.humidityValid, r.humidity, "%%%.0f");
    format_log_value(volt, sizeof(volt), r.voltageValid, r.voltage, "%.2f V");
    format_log_value(cur, sizeof(cur), r.currentValid, r.current, "%.2f A");
    format_log_value(pwr, sizeof(pwr), r.powerValid, r.power, "%.1f W");

    // Zaman damgasi "YYYY-MM-DD HH:MM:SS"; saat kismi (11. karakterden sonrasi) buyuk gosterilir.
    lv_obj_t *row = lv_label_create(s_log_view_list);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_style_bg_color(row, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_pad_all(row, 6, 0);
    lv_label_set_recolor(row, true);
    snprintf(buf, sizeof(buf), "#22d3ee %s#\n%s   Nem %s\n%s  %s  %s",
             r.timestamp + 11, temp, hum, volt, cur, pwr);
    lv_label_set_text(row, buf);
  }
}

// ui_page_log_update() icinden her saniye cagrilir.
static void log_view_update()
{
  if (s_log_view_list == NULL) {
    return;
  }
  if (g_logView.version != s_log_view_rendered_version) {
    s_log_view_rendered_version = g_logView.version;
    s_log_view_waiting = false;
    log_view_render();
  }
  // Yeni satir yazildiysa listeyi tazele. Onceki yukleme cizilmeden yenisi
  // istenmez: boylece Driver_Loop, UI okurken g_logView'a yazmaz.
  if (!s_log_view_waiting && g_loggingStatus.rowCount != s_log_view_seen_row_count) {
    log_view_request_reload();
  }
}

static void open_log_view_cb(lv_event_t *e)
{
  s_log_view_list = ui_modal_open("KAYITLAR");
  lv_obj_add_event_cb(s_log_view_list, log_view_deleted_cb, LV_EVENT_DELETE, NULL);

  lv_obj_t *loading = lv_label_create(s_log_view_list);
  lv_label_set_text(loading, "SD karttan okunuyor...");
  ui_theme_style_subtitle(loading);

  s_log_view_rendered_version = g_logView.version;
  log_view_request_reload();
}

void ui_page_log_create(lv_obj_t *tile)
{
  lv_obj_t *content = lv_obj_create(tile);
  lv_obj_set_size(content, UI_CONTENT_WIDTH, UI_CONTENT_HEIGHT);
  lv_obj_align(content, LV_ALIGN_TOP_MID, 0, UI_CONTENT_TOP);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_pad_all(content, 0, 0);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(content);
  lv_label_set_text(title, "VERI KAYDI");
  ui_theme_style_title(title);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_26, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t *info_card = ui_theme_create_card(content);
  lv_obj_set_size(info_card, LV_PCT(100), 88);
  lv_obj_align_to(info_card, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
  lv_obj_set_flex_flow(info_card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(info_card, 1, 0);

  s_log_sd_label = lv_label_create(info_card);
  ui_theme_style_subtitle(s_log_sd_label);
  lv_obj_set_style_text_color(s_log_sd_label, UI_COLOR_TEXT, 0);

  s_log_active_label = lv_label_create(info_card);
  ui_theme_style_subtitle(s_log_active_label);
  lv_obj_set_style_text_color(s_log_active_label, UI_COLOR_TEXT, 0);

  s_log_file_label = lv_label_create(info_card);
  ui_theme_style_subtitle(s_log_file_label);
  lv_obj_set_style_text_color(s_log_file_label, UI_COLOR_TEXT, 0);

  s_log_rows_label = lv_label_create(info_card);
  ui_theme_style_subtitle(s_log_rows_label);
  lv_obj_set_style_text_color(s_log_rows_label, UI_COLOR_TEXT, 0);

  s_log_last_write_label = lv_label_create(info_card);
  ui_theme_style_subtitle(s_log_last_write_label);
  lv_obj_set_style_text_color(s_log_last_write_label, UI_COLOR_TEXT, 0);

  s_log_list = lv_obj_create(content);
  lv_obj_set_size(s_log_list, LV_PCT(100), 70);
  lv_obj_align_to(s_log_list, info_card, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
  lv_obj_set_style_bg_color(s_log_list, UI_COLOR_CARD, 0);
  lv_obj_set_style_border_color(s_log_list, UI_COLOR_CARD_BORDER, 0);
  lv_obj_set_style_border_width(s_log_list, 1, 0);
  lv_obj_set_style_radius(s_log_list, 12, 0);
  lv_obj_set_flex_flow(s_log_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(s_log_list, 6, 0);
  lv_obj_set_style_pad_row(s_log_list, 2, 0);

  lv_obj_t *view_btn = lv_btn_create(content);
  lv_obj_set_size(view_btn, LV_PCT(100), 40);
  lv_obj_align_to(view_btn, s_log_list, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
  lv_obj_set_style_bg_color(view_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(view_btn, open_log_view_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *view_label = lv_label_create(view_btn);
  lv_label_set_text(view_label, LV_SYMBOL_LIST " KAYDI IZLE");
  lv_obj_center(view_label);

  lv_obj_t *btn_row = lv_obj_create(content);
  lv_obj_set_size(btn_row, LV_PCT(100), 44);
  lv_obj_align(btn_row, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(btn_row, 0, 0);
  lv_obj_set_style_pad_all(btn_row, 0, 0);
  lv_obj_set_style_pad_column(btn_row, 8, 0);
  lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);

  s_log_start_stop_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(s_log_start_stop_btn, 1);
  lv_obj_set_height(s_log_start_stop_btn, 44);
  lv_obj_set_style_bg_color(s_log_start_stop_btn, UI_COLOR_ACCENT_FILL, 0);
  lv_obj_add_event_cb(s_log_start_stop_btn, log_start_stop_btn_cb, LV_EVENT_CLICKED, NULL);
  s_log_start_stop_label = lv_label_create(s_log_start_stop_btn);
  lv_label_set_text(s_log_start_stop_label, "BASLAT");
  lv_obj_center(s_log_start_stop_label);

  lv_obj_t *save_now_btn = lv_btn_create(btn_row);
  lv_obj_set_flex_grow(save_now_btn, 1);
  lv_obj_set_height(save_now_btn, 44);
  lv_obj_set_style_bg_color(save_now_btn, UI_COLOR_CARD_BORDER, 0);
  lv_obj_add_event_cb(save_now_btn, log_save_now_btn_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *save_now_label = lv_label_create(save_now_btn);
  lv_label_set_text(save_now_label, "SIMDI KAYDET");
  lv_obj_center(save_now_label);
}

void ui_page_log_update()
{
  char buf[64];

  snprintf(buf, sizeof(buf), "SD: %s", g_systemStatus.sdMounted ? "hazir" : "yok");
  lv_label_set_text(s_log_sd_label, buf);

  snprintf(buf, sizeof(buf), "Kayit: %s", g_loggingStatus.enabled ? "aktif" : "kapali");
  lv_label_set_text(s_log_active_label, buf);
  lv_label_set_text(s_log_start_stop_label, g_loggingStatus.enabled ? "DURDUR" : "BASLAT");

  snprintf(buf, sizeof(buf), "Dosya: %s", g_loggingStatus.activeFilename[0] ? g_loggingStatus.activeFilename : "-");
  lv_label_set_text(s_log_file_label, buf);

  snprintf(buf, sizeof(buf), "Satir: %lu", (unsigned long)g_loggingStatus.rowCount);
  lv_label_set_text(s_log_rows_label, buf);

  if (g_loggingStatus.lastWriteMs == 0) {
    lv_label_set_text(s_log_last_write_label, "Son yazma: -");
  } else {
    snprintf(buf, sizeof(buf), "Son yazma: %s", g_loggingStatus.lastWriteSuccessful ? "basarili" : "basarisiz");
    lv_label_set_text(s_log_last_write_label, buf);
  }

  // Olay listesi yalnizca gercekten degistiyse yeniden olusturulur
  // (her saniye tum ekrani/listeyi yeniden cizmemek icin).
  static uint32_t s_last_rendered_events_version = 0xFFFFFFFF;
  if (g_loggingStatus.eventsVersion != s_last_rendered_events_version) {
    s_last_rendered_events_version = g_loggingStatus.eventsVersion;

    lv_obj_clean(s_log_list);
    for (uint8_t i = 0; i < g_loggingStatus.eventCount; i++) {
      lv_obj_t *row = lv_label_create(s_log_list);
      char line[LOG_EVENT_TEXT_LEN + 8];
      snprintf(line, sizeof(line), "%s %s", g_loggingStatus.events[i].timeText, g_loggingStatus.events[i].text);
      lv_label_set_text(row, line);
      ui_theme_style_subtitle(row);
      lv_obj_set_style_text_color(row, UI_COLOR_TEXT, 0);
    }
    if (g_loggingStatus.eventCount == 0) {
      lv_obj_t *row = lv_label_create(s_log_list);
      lv_label_set_text(row, "Henuz kayit yok");
      ui_theme_style_subtitle(row);
    }
  }

  log_view_update();
}
