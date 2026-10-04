#pragma once

// secrets.h yoksa (ör. .gitignore nedeniyle repoya eklenmediyse) derleme
// hata vermez; Wi-Fi/MQTT baglantisi devre disi kalacak sekilde guvenli,
// bos bir demo yapilandirmasiyla derlenir.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "secrets.h bulunamadi: guvenli demo yapilandirmasi kullaniliyor (Wi-Fi/MQTT devre disi)"
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define MQTT_HOST "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#endif
