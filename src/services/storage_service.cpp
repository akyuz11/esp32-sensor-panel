#include "storage_service.h"
#include "app_state.h"
#include "board.h"

static bool s_mounted = false;
static uint32_t s_last_remount_attempt_ms = 0;
#define REMOUNT_RETRY_INTERVAL_MS 10000

static void refresh_from_card()
{
  s_mounted = board_sd_card_present();

  g_systemStatus.sdMounted = s_mounted;
  if (s_mounted) {
    g_systemStatus.sdCapacityBytes = board_sd_total_bytes();
    g_systemStatus.sdUsedBytes = board_sd_used_bytes();
  } else {
    g_systemStatus.sdCapacityBytes = 0;
    g_systemStatus.sdUsedBytes = 0;
  }
}

void storage_service_init()
{
  board_sd_begin();
  refresh_from_card();
  if (!s_mounted) {
    Serial.println("[SD] Kart bulunamadi, cihaz calismaya devam edecek");
  }
}

void storage_service_update()
{
  if (s_mounted) {
    refresh_from_card();
    if (!s_mounted) {
      Serial.println("[SD] Kart cikarildi");
    }
    return;
  }

  // Kart takili degil: makul araliklarla yeniden baglanmayi dene.
  uint32_t now = millis();
  if (now - s_last_remount_attempt_ms < REMOUNT_RETRY_INTERVAL_MS) {
    return;
  }
  s_last_remount_attempt_ms = now;

  board_sd_end();
  board_sd_begin();
  refresh_from_card();
  if (s_mounted) {
    Serial.println("[SD] Kart yeniden baglandi");
  }
}

bool storage_service_is_mounted()
{
  return s_mounted;
}

void storage_service_run_boot_test()
{
  if (!s_mounted) {
    Serial.println("[SD TEST] Kart takili degil, deneme atlandi");
    return;
  }

  const char *path = "/deneme.txt";

  // 1) Onceki acilislari say: dosyadaki satir sayisi = onceki acilis sayisi
  uint32_t boot_count = 1;
  File rf = board_sd_fs().open(path, FILE_READ);
  if (rf) {
    while (rf.available()) {
      if (rf.read() == '\n') {
        boot_count++;
      }
    }
    rf.close();
  }

  // 2) Yeni satiri dosyanin sonuna ekle
  File wf = board_sd_fs().open(path, FILE_APPEND);
  if (!wf) {
    Serial.println("[SD TEST] Dosya yazmak icin acilamadi");
    return;
  }
  wf.printf("Acilis #%lu - millis=%lu\n", (unsigned long)boot_count, (unsigned long)millis());
  wf.close();
  Serial.printf("[SD TEST] Yazildi -> %s (Acilis #%lu)\r\n", path, (unsigned long)boot_count);

  // 3) Dosyanin tamamini geri oku ve seri monitore bas
  rf = board_sd_fs().open(path, FILE_READ);
  if (!rf) {
    Serial.println("[SD TEST] Dosya okumak icin acilamadi");
    return;
  }
  Serial.println("[SD TEST] ---- deneme.txt icerigi ----");
  while (rf.available()) {
    Serial.write(rf.read());
  }
  rf.close();
  Serial.println("[SD TEST] ---------------------------");
}
