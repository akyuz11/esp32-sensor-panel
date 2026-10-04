#include "logging_service.h"
#include "app_state.h"
#include "app_config.h"
#include "board/waveshare/SD_Card.h"
#include "board/waveshare/RTC_PCF85063.h"

#include <stdio.h>
#include <stdlib.h>

static uint32_t s_last_check_ms = 0;
static volatile bool s_write_now_requested = false;
static volatile bool s_view_reload_requested = false;

static void format_current_time_hhmm(char *out, size_t out_len)
{
  if (g_systemStatus.rtcValid) {
    snprintf(out, out_len, "%02u:%02u", datetime.hour, datetime.minute);
  } else {
    snprintf(out, out_len, "--:--");
  }
}

static void append_field(char *line, size_t line_size, bool valid, float value)
{
  char field[16];
  if (valid) {
    snprintf(field, sizeof(field), "%.2f", value);
  } else {
    field[0] = '\0'; // Gecersiz deger: bos CSV alani (nan yazma)
  }
  strncat(line, field, line_size - strlen(line) - 1);
  strncat(line, ",", line_size - strlen(line) - 1);
}

static bool write_row_now()
{
  char time_text[6];
  format_current_time_hhmm(time_text, sizeof(time_text));

  if (!g_systemStatus.sdMounted) {
    g_loggingStatus.lastWriteSuccessful = false;
    strncpy(g_loggingStatus.lastError, "SD kart yok", sizeof(g_loggingStatus.lastError) - 1);
    g_loggingStatus.pushEvent(time_text, "SD yok, kayit atlandi");
    return false;
  }

  if (!g_systemStatus.rtcValid) {
    g_loggingStatus.lastWriteSuccessful = false;
    strncpy(g_loggingStatus.lastError, "Saat gecersiz", sizeof(g_loggingStatus.lastError) - 1);
    g_loggingStatus.pushEvent(time_text, "Saat gecersiz, kayit atlandi");
    return false;
  }

  if (!SD_MMC.exists("/logs")) {
    SD_MMC.mkdir("/logs");
  }

  char path[48];
  snprintf(path, sizeof(path), "/logs/%04u-%02u-%02u.csv", datetime.year, datetime.month, datetime.day);

  bool file_exists = SD_MMC.exists(path);

  File f = SD_MMC.open(path, FILE_APPEND);
  if (!f) {
    g_loggingStatus.lastWriteSuccessful = false;
    strncpy(g_loggingStatus.lastError, "Dosya acilamadi", sizeof(g_loggingStatus.lastError) - 1);
    g_loggingStatus.pushEvent(time_text, "Kayit basarisiz");
    Serial.printf("[LOG] Dosya acilamadi: %s\r\n", path);
    return false;
  }

  if (!file_exists) {
    f.println("timestamp,temperature_c,humidity_pct,voltage_v,current_a,power_w,wifi_rssi,mqtt_connected");
  }

  char line[160];
  snprintf(line, sizeof(line), "%04u-%02u-%02u %02u:%02u:%02u,",
           datetime.year, datetime.month, datetime.day,
           datetime.hour, datetime.minute, datetime.second);

  append_field(line, sizeof(line), g_sensorValues.temperatureValid, g_sensorValues.temperature);
  append_field(line, sizeof(line), g_sensorValues.humidityValid, g_sensorValues.humidity);
  append_field(line, sizeof(line), g_sensorValues.voltageValid, g_sensorValues.voltage);
  append_field(line, sizeof(line), g_sensorValues.currentValid, g_sensorValues.current);
  append_field(line, sizeof(line), g_sensorValues.powerValid, g_sensorValues.power);

  char tail[16];
  snprintf(tail, sizeof(tail), "%d,%d", g_systemStatus.wifiRssi, g_systemStatus.mqttConnected ? 1 : 0);
  strncat(line, tail, sizeof(line) - strlen(line) - 1);

  f.println(line);
  f.close();

  strncpy(g_loggingStatus.activeFilename, path, sizeof(g_loggingStatus.activeFilename) - 1);
  g_loggingStatus.lastWriteSuccessful = true;
  g_loggingStatus.lastError[0] = '\0';
  g_loggingStatus.rowCount++;
  g_loggingStatus.lastWriteMs = millis();
  g_loggingStatus.pushEvent(time_text, "Kayit basarili");
  Serial.printf("[LOG] Row written successfully -> %s\r\n", path);
  return true;
}

// ---------------------------------------------------------------------------
// "KAYDI IZLE": en yeni CSV dosyasinin sonunu okuyup g_logView'a yazar.
// ---------------------------------------------------------------------------

// Dosyanin yalnizca son kismi okunur (bir satir ~55 bayt); dosya ne kadar
// buyurse buyusun bellek kullanimi sabit kalir.
#define LOG_VIEW_TAIL_BYTES  (LOG_VIEW_MAX_ROWS * 96)
#define LOG_VIEW_PARSE_MAX   (LOG_VIEW_MAX_ROWS * 2)

static LogViewRow s_parsed_rows[LOG_VIEW_PARSE_MAX];

// /logs altindaki en yeni CSV (ad YYYY-MM-DD.csv oldugu icin alfabetik en
// buyuk ad = en yeni gun).
static bool find_newest_log_file(char *out, size_t out_len)
{
  File dir = SD_MMC.open("/logs");
  if (!dir || !dir.isDirectory()) {
    return false;
  }
  char newest[32] = "";
  File entry = dir.openNextFile();
  while (entry) {
    const char *name = entry.name();
    size_t len = strlen(name);
    if (!entry.isDirectory() && len > 4 && len < sizeof(newest) && strcmp(name + len - 4, ".csv") == 0 &&
        strcmp(name, newest) > 0) {
      strcpy(newest, name);
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  if (newest[0] == '\0') {
    return false;
  }
  snprintf(out, out_len, "/logs/%s", newest);
  return true;
}

static bool parse_float_field(const char *text, float *out)
{
  if (text[0] == '\0') {
    return false; // bos alan = o anda gecersiz deger
  }
  char *end = NULL;
  *out = strtof(text, &end);
  return end != text;
}

// "timestamp,temp,hum,volt,cur,pow,rssi,mqtt" satirini ayristirir. strtok
// kullanilmaz: bos alanlari (gecersiz degerleri) atlamamasi gerekir.
static bool parse_csv_line(char *line, LogViewRow *row)
{
  char *fields[8];
  int count = 0;
  char *p = line;
  fields[count++] = p;
  while (*p != '\0' && count < 8) {
    if (*p == ',') {
      *p = '\0';
      fields[count++] = p + 1;
    }
    p++;
  }
  if (count < 6 || strlen(fields[0]) != 19) {
    return false; // baslik satiri veya bozuk/yarim satir
  }

  memcpy(row->timestamp, fields[0], 19);
  row->timestamp[19] = '\0';
  row->temperatureValid = parse_float_field(fields[1], &row->temperature);
  row->humidityValid = parse_float_field(fields[2], &row->humidity);
  row->voltageValid = parse_float_field(fields[3], &row->voltage);
  row->currentValid = parse_float_field(fields[4], &row->current);
  row->powerValid = parse_float_field(fields[5], &row->power);
  return true;
}

static int compare_rows_newest_first(const void *a, const void *b)
{
  return strcmp(((const LogViewRow *)b)->timestamp, ((const LogViewRow *)a)->timestamp);
}

static void finish_view(const char *path, uint8_t count, const char *error)
{
  for (uint8_t i = 0; i < count; i++) {
    g_logView.rows[i] = s_parsed_rows[i];
  }
  g_logView.rowCount = count;
  strncpy(g_logView.filename, path, sizeof(g_logView.filename) - 1);
  g_logView.filename[sizeof(g_logView.filename) - 1] = '\0';
  strncpy(g_logView.error, error, sizeof(g_logView.error) - 1);
  g_logView.error[sizeof(g_logView.error) - 1] = '\0';
  g_logView.version++;
}

static void load_view()
{
  if (!g_systemStatus.sdMounted) {
    finish_view("", 0, "SD kart yok");
    return;
  }

  char path[48];
  if (!find_newest_log_file(path, sizeof(path))) {
    finish_view("", 0, "Henuz kayit dosyasi yok");
    return;
  }

  File f = SD_MMC.open(path, FILE_READ);
  if (!f) {
    finish_view(path, 0, "Dosya acilamadi");
    return;
  }

  size_t size = f.size();
  size_t start = size > LOG_VIEW_TAIL_BYTES ? size - LOG_VIEW_TAIL_BYTES : 0;
  size_t len = size - start;
  char *buf = (char *)malloc(len + 1);
  if (buf == NULL) {
    f.close();
    finish_view(path, 0, "Bellek yetersiz");
    return;
  }
  f.seek(start);
  len = f.read((uint8_t *)buf, len);
  f.close();
  buf[len] = '\0';

  char *line = buf;
  if (start > 0) {
    // Dosyanin ortasindan baslandi: ilk (yarim) satiri atla.
    char *nl = strchr(line, '\n');
    line = nl ? nl + 1 : buf + len;
  }

  uint8_t count = 0;
  while (*line != '\0') {
    char *nl = strchr(line, '\n');
    if (nl) {
      *nl = '\0';
    }
    size_t line_len = strlen(line);
    if (line_len > 0 && line[line_len - 1] == '\r') {
      line[line_len - 1] = '\0';
    }

    LogViewRow row;
    if (parse_csv_line(line, &row)) {
      if (count == LOG_VIEW_PARSE_MAX) {
        // Tampon dolu: en eskiyi at (dosya zaten kronolojik, yeniler sonda).
        memmove(&s_parsed_rows[0], &s_parsed_rows[1], sizeof(LogViewRow) * (LOG_VIEW_PARSE_MAX - 1));
        count--;
      }
      s_parsed_rows[count++] = row;
    }

    if (!nl) {
      break;
    }
    line = nl + 1;
  }
  free(buf);

  // Zaman damgasina gore en yeni basta (saat NTP ile duzeltildiyse dosya
  // sirasi ile zaman sirasi farkli olabilir; bu yuzden acikca siralanir).
  qsort(s_parsed_rows, count, sizeof(LogViewRow), compare_rows_newest_first);
  if (count > LOG_VIEW_MAX_ROWS) {
    count = LOG_VIEW_MAX_ROWS;
  }

  finish_view(path, count, count == 0 ? "Dosyada kayit yok" : "");
  Serial.printf("[LOG] Izleme listesi yuklendi: %s (%u satir)\r\n", path, count);
}

void logging_service_init()
{
  g_loggingStatus.intervalSeconds = APP_LOG_INTERVAL_SECONDS;
  g_loggingStatus.enabled = false;
}

void logging_service_loop()
{
  if (s_write_now_requested) {
    s_write_now_requested = false;
    write_row_now();
  }

  if (s_view_reload_requested) {
    s_view_reload_requested = false;
    load_view();
  }

  if (!g_loggingStatus.enabled) {
    return;
  }

  uint32_t now = millis();
  if (now - s_last_check_ms < (uint32_t)g_loggingStatus.intervalSeconds * 1000UL) {
    return;
  }
  s_last_check_ms = now;
  write_row_now();
}

void logging_service_start()
{
  g_loggingStatus.enabled = true;
  s_last_check_ms = millis() - (uint32_t)g_loggingStatus.intervalSeconds * 1000UL; // ilk kaydi hemen dene
  Serial.println("[LOG] Kayit baslatildi");
}

void logging_service_stop()
{
  g_loggingStatus.enabled = false;
  Serial.println("[LOG] Kayit durduruldu");
}

void logging_service_request_write_now()
{
  s_write_now_requested = true;
}

void logging_service_request_view_reload()
{
  s_view_reload_requested = true;
}
