#pragma once
// Android uygulamasi ("Deney Karti") icin BLE GATT sunucusu.
//
// Servis 87ba0001-...:
//   87ba0002 NOTIFY       - hareket paketi (v1, 20 bayt, 200 ms)
//   87ba0003 READ|NOTIFY  - sensor + durum paketi (v2, 20 bayt, 1 sn)
//   87ba0004 WRITE        - telefondan komut (ilk bayt = komut kodu)
// Paket bicimleri ble_service.cpp icinde ve Android tarafinda Packet.java /
// SensorPacket.java icinde birebir ayni tanimlidir.
void ble_service_init();
// Called only by Driver_Loop, after motion_service_loop.
void ble_service_loop();
