"""Panelin VERILER sayfasini test etmek icin sahte sensor verisi yayinlar.

Kullanim:
    python tools/mqtt_simulator.py            # 3 sn'de bir yayin
    python tools/mqtt_simulator.py --once     # tek sefer yayinla ve cik
    python tools/mqtt_simulator.py --interval 1

Topic'ler include/app_config.h'teki MQTT_TOPIC_* ile ayni olmali; payload sade
sayidir (ornek: "24.6"). retain=True ile yayinlanir, boylece panel sonradan
baglansa bile son degeri hemen alir.
"""

import argparse
import math
import random
import time

import paho.mqtt.client as mqtt

BROKER = "test.mosquitto.org"
PORT = 1883
PREFIX = "MQTT_Verileri"

TOPICS = {
    "temperature": f"{PREFIX}/sensor/temperature",
    "humidity": f"{PREFIX}/sensor/humidity",
    "voltage": f"{PREFIX}/energy/voltage",
    "current": f"{PREFIX}/energy/current",
    "power": f"{PREFIX}/energy/power",
}


def make_values(t):
    """Yavasca salinan, biraz gurultulu gercekci degerler uretir."""
    temperature = 24.0 + 3.0 * math.sin(t / 60.0) + random.uniform(-0.2, 0.2)
    humidity = 50.0 + 10.0 * math.sin(t / 90.0 + 1.0) + random.uniform(-0.5, 0.5)
    voltage = 230.0 + 4.0 * math.sin(t / 45.0) + random.uniform(-0.5, 0.5)
    current = 1.5 + 0.8 * math.sin(t / 30.0 + 2.0) + random.uniform(-0.05, 0.05)
    power = voltage * current
    return {
        "temperature": f"{temperature:.1f}",
        "humidity": f"{humidity:.1f}",
        "voltage": f"{voltage:.1f}",
        "current": f"{current:.2f}",
        "power": f"{power:.0f}",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--interval", type=float, default=3.0, help="yayin araligi (sn)")
    parser.add_argument("--once", action="store_true", help="tek sefer yayinla ve cik")
    args = parser.parse_args()

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect(BROKER, PORT, keepalive=30)
    client.loop_start()
    print(f"{BROKER}:{PORT} adresine baglanildi, onek: {PREFIX}/ (durdurmak icin Ctrl+C)")

    start = time.time()
    try:
        while True:
            values = make_values(time.time() - start)
            for key, payload in values.items():
                client.publish(TOPICS[key], payload, qos=0, retain=True).wait_for_publish(5)
            print("  ".join(f"{k}={v}" for k, v in values.items()))
            if args.once:
                break
            time.sleep(args.interval)
    except KeyboardInterrupt:
        pass
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
