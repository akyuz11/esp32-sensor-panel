#pragma once

// PubSubClient uzerinden MQTT baglantisi. Yalnizca Wi-Fi bagliyken calisir.
// Sade sayisal payload'lari (ör. "24.6") kabul eder; JSON yok.
// Tum aginetworksok/baglanti islemleri arka plan gorevinde (main.cpp'nin
// Driver_Loop'u) calisir, LVGL geri cagrilari yalnizca istek bayragi set eder.

void mqtt_service_init();
void mqtt_service_loop();

void mqtt_service_request_connect();
void mqtt_service_request_disconnect();
