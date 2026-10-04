package tr.deney.espmonitor;
import java.nio.*;
// Karttaki ble_service.cpp::send_sensors() ile birebir ayni v2 paket bicimi.
final class SensorPacket {
    final float temperature, humidity, voltage, current, power;
    final boolean temperatureValid, humidityValid, voltageValid, currentValid, powerValid;
    final boolean demo, wifi, mqtt, sd, rtc, logging, batteryValid, lastLogOk;
    final int batteryPercent, logRows, brightness, ageSeconds;
    SensorPacket(byte[] bytes) {
        if (bytes == null || bytes.length != 20 || bytes[0] != 2)
            throw new IllegalArgumentException("Uyumsuz sensör paketi");
        ByteBuffer b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        b.get();
        int f1 = b.get() & 255, f2 = b.get() & 255;
        temperatureValid = (f1 & 1) != 0; humidityValid = (f1 & 2) != 0; voltageValid = (f1 & 4) != 0;
        currentValid = (f1 & 8) != 0; powerValid = (f1 & 16) != 0; demo = (f1 & 32) != 0;
        wifi = (f1 & 64) != 0; mqtt = (f1 & 128) != 0;
        sd = (f2 & 1) != 0; rtc = (f2 & 2) != 0; logging = (f2 & 4) != 0;
        batteryValid = (f2 & 8) != 0; lastLogOk = (f2 & 16) != 0;
        batteryPercent = b.get() & 255;
        temperature = b.getShort() / 100f;
        humidity = Short.toUnsignedInt(b.getShort()) / 100f;
        voltage = Short.toUnsignedInt(b.getShort()) / 100f;
        current = b.getShort() / 100f;
        power = b.getInt() / 10f;
        logRows = Short.toUnsignedInt(b.getShort());
        brightness = b.get() & 255;
        ageSeconds = b.get() & 255;
    }
}
