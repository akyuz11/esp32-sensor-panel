package tr.deney.espmonitor;
import java.nio.*;
final class Packet {
    final float x,y,z,gx,gy,gz,voltage;
    final long uptime;
    final boolean batteryValid;
    Packet(byte[] bytes) {
        if (bytes == null || bytes.length != 20 || bytes[0] != 1)
            throw new IllegalArgumentException("Uyumsuz veri paketi");
        ByteBuffer b = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        b.get(); batteryValid = (b.get() & 1) != 0;
        uptime = Integer.toUnsignedLong(b.getInt());
        x=b.getShort()/1000f; y=b.getShort()/1000f; z=b.getShort()/1000f;
        gx=b.getShort()/100f; gy=b.getShort()/100f; gz=b.getShort()/100f;
        voltage=Short.toUnsignedInt(b.getShort())/1000f;
    }
    float tiltX() { return (float)Math.toDegrees(Math.atan2(x, Math.hypot(y,z))); }
    float tiltY() { return (float)Math.toDegrees(Math.atan2(y, Math.hypot(x,z))); }
}