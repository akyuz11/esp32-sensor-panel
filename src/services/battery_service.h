#pragma once

// Waveshare'in resmi ornegindeki BAT_Driver (BAT_Get_Volts / BAT_ADC_PIN)
// uzerinden okunan gerilimi basit, dogrusal olmayan bir Li-Po tablosuyla
// yuzdeye cevirip g_systemStatus'a yazar. Bu yuzde yalnizca yaklasik bir
// gostergedir; kartta gercek bir sarj/pil-var mi pini bulunmuyor.
void battery_service_update();
