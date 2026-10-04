#pragma once

// QMI8658 ivme verisini (vendor QMI8658_Loop() tarafindan Accel global'ine
// yazilir) yorumlar: son ivme degerlerini ve sallama olaylarini
// g_motionStatus'a yazar. Yalnizca Driver_Loop icinden cagrilir.

void motion_service_init();

// Driver_Loop icinde QMI8658_Loop()'tan hemen sonra cagrilir (~10 Hz).
void motion_service_loop();
