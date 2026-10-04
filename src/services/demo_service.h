#pragma once

// DEMO_MODE: MQTT verisi gelmeden VERILER sayfasini test edebilmek icin
// yavasca degisen ornek sensor degerleri uretir. g_sensorValues.demoMode
// true oldugu surece calisir; gercek MQTT verisi gelince (Asama 4)
// demoMode false yapilir ve bu fonksiyon deger uretmeyi birakir.
void demo_service_loop();
