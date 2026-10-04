#pragma once

// Telefondan izlenebilen "su terazisi" web sayfasi. Kart Wi-Fi'ye baglaninca
// port 80'de kucuk bir HTTP sunucusu acar:
//   /      -> baloncuklu su terazisi sayfasi (tarayicida calisir)
//   /data  -> anlik ivme degerleri (JSON), sayfa bunu surekli sorgular
//
// Sunucu kendi FreeRTOS gorevinde calisir: yavas bir istemci Driver_Loop'u
// (sensor/SD/MQTT) ya da LVGL'yi bekletmez. Yalnizca g_motionStatus okunur.

void web_service_init();
