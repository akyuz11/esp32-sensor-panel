#include "web_service.h"
#include "app_state.h"

#include <Arduino.h>
#include <WebServer.h>

static WebServer s_server(80);

static const char PAGE_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="tr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Su Terazisi</title>
<style>
  body{margin:0;background:#0b1024;color:#eaf0ff;font-family:sans-serif;text-align:center}
  h1{font-size:22px;margin:16px 0 4px}
  #level{position:relative;width:80vw;max-width:360px;aspect-ratio:1;margin:12px auto;
         border-radius:50%;background:radial-gradient(#1c2a55,#0e1530);border:3px solid #25304f}
  .ring{position:absolute;left:50%;top:50%;border:2px solid #3a4a7a;border-radius:50%;transform:translate(-50%,-50%)}
  #bubble{position:absolute;left:50%;top:50%;width:18%;aspect-ratio:1;border-radius:50%;
          background:#22d3ee;box-shadow:0 0 18px #22d3ee;transform:translate(-50%,-50%);transition:all .12s linear}
  #bubble.flat{background:#34d399;box-shadow:0 0 22px #34d399}
  .vals{display:flex;justify-content:center;gap:28px;font-size:34px;font-weight:bold}
  .vals small{display:block;font-size:14px;color:#8a96b8;font-weight:normal}
  #state{font-size:22px;margin:10px;min-height:28px}
  #raw{color:#8a96b8;font-size:13px}
</style></head><body>
<h1>ESP32 Su Terazisi</h1>
<div id="level">
  <div class="ring" style="width:66%;height:66%"></div>
  <div class="ring" style="width:22%;height:22%"></div>
  <div id="bubble"></div>
</div>
<div class="vals">
  <div><small>X egimi</small><span id="ax">-</span></div>
  <div><small>Y egimi</small><span id="ay">-</span></div>
</div>
<div id="state">Baglaniliyor...</div>
<div id="raw"></div>
<script>
const bubble=document.getElementById('bubble');
function deg(r){return r*180/Math.PI}
async function tick(){
  try{
    const d=await (await fetch('/data',{cache:'no-store'})).json();
    // Kart duz dururken yercekimi Z ekseninde (~1 g); X/Y egimi, o eksendeki
    // yercekimi payindan hesaplanir.
    const tx=deg(Math.atan2(d.x,Math.hypot(d.y,d.z)));
    const ty=deg(Math.atan2(d.y,Math.hypot(d.x,d.z)));
    document.getElementById('ax').textContent=tx.toFixed(1)+'°';
    document.getElementById('ay').textContent=ty.toFixed(1)+'°';
    // Baloncuk yuksek tarafa kacar; 30 derece egim = dis kenar.
    const lim=30, k=41/lim;
    let px=Math.max(-lim,Math.min(lim,tx))*k, py=Math.max(-lim,Math.min(lim,ty))*k;
    bubble.style.left=(50+px)+'%'; bubble.style.top=(50+py)+'%';
    const flat=Math.abs(tx)<1&&Math.abs(ty)<1;
    bubble.classList.toggle('flat',flat);
    document.getElementById('state').textContent=flat?'DUZ ✔':'Egik';
    document.getElementById('raw').textContent=
      'ivme (g)  x='+d.x.toFixed(2)+'  y='+d.y.toFixed(2)+'  z='+d.z.toFixed(2);
  }catch(e){
    document.getElementById('state').textContent='Baglanti yok, tekrar deneniyor...';
  }
  setTimeout(tick,150);
}
tick();
</script></body></html>)HTML";

static void handle_root()
{
  s_server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

static void handle_data()
{
  char json[80];
  snprintf(json, sizeof(json), "{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f}",
           g_motionStatus.accelX, g_motionStatus.accelY, g_motionStatus.accelZ);
  s_server.send(200, "application/json", json);
}

static void web_task(void *parameter)
{
  // Ag yigini hazir olmadan dinlemeye baslama: ilk Wi-Fi baglantisini bekle.
  while (!g_systemStatus.wifiConnected) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  s_server.on("/", handle_root);
  s_server.on("/data", handle_data);
  s_server.begin();
  Serial.printf("[WEB] Su terazisi: http://%s/\r\n", g_systemStatus.wifiIp);

  while (1) {
    s_server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void web_service_init()
{
  xTaskCreatePinnedToCore(web_task, "WebServer", 6144, NULL, 1, NULL, 0);
}
