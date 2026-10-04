#include "motion_service.h"
#include "app_state.h"
#include "app_config.h"
#include "board/waveshare/Gyro_QMI8658.h"

#include <Arduino.h>
#include <math.h>

// Ornekleme ~10 Hz oldugu icin tek bir ivme tepesini yakalamak guvenilir
// degil; bunun yerine ardisik iki okuma arasindaki vektorel farka bakilir.
// Yavasca egmek bu esigi asmaz, sert bir sallama ise asar.
static float s_prev_x, s_prev_y, s_prev_z;
static bool s_has_prev = false;
static uint32_t s_first_hit_ms = 0;
static uint8_t s_hit_count = 0;
static uint32_t s_cooldown_until_ms = 0;

void motion_service_init()
{
  s_has_prev = false;
  s_hit_count = 0;
  s_cooldown_until_ms = 0;
}

void motion_service_loop()
{
  float x = Accel.x, y = Accel.y, z = Accel.z;
  g_motionStatus.accelX = x;
  g_motionStatus.accelY = y;
  g_motionStatus.accelZ = z;

  if (!s_has_prev) {
    s_prev_x = x; s_prev_y = y; s_prev_z = z;
    s_has_prev = true;
    return;
  }

  float dx = x - s_prev_x, dy = y - s_prev_y, dz = z - s_prev_z;
  float delta = sqrtf(dx * dx + dy * dy + dz * dz);
  s_prev_x = x; s_prev_y = y; s_prev_z = z;

  uint32_t now = millis();
  if (now < s_cooldown_until_ms) {
    return;
  }

  // Pencere disina tasan tek vuruslar unutulur.
  if (s_hit_count > 0 && now - s_first_hit_ms > APP_SHAKE_WINDOW_MS) {
    s_hit_count = 0;
  }

  if (delta >= APP_SHAKE_DELTA_G) {
    if (s_hit_count == 0) {
      s_first_hit_ms = now;
    }
    s_hit_count++;
    if (s_hit_count >= APP_SHAKE_HITS) {
      s_hit_count = 0;
      s_cooldown_until_ms = now + APP_SHAKE_COOLDOWN_MS;
      g_motionStatus.shakeCount++;
      Serial.printf("[MOTION] Sallama algilandi (#%lu, delta=%.2f g)\r\n",
          (unsigned long)g_motionStatus.shakeCount, delta);
    }
  }
}
