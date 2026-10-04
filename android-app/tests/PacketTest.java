package tr.deney.espmonitor;
import java.nio.*;
public class PacketTest {
    static void check(boolean ok) { if(!ok) throw new AssertionError(); }
    public static void main(String[] args) {
        byte[] bytes=new byte[20];
        ByteBuffer b=ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        b.put((byte)1).put((byte)1).putInt(-1);
        b.putShort((short)-1000).putShort((short)0).putShort((short)1000);
        b.putShort((short)-1234).putShort((short)567).putShort((short)0).putShort((short)4100);
        Packet p=new Packet(bytes);
        check(p.uptime==4294967295L);
        check(p.batteryValid && Math.abs(p.voltage-4.1)<.001);
        check(p.x==-1 && p.z==1 && Math.abs(p.gx+12.34)<.001);
        check(Math.abs(p.tiltX()+45)<.001 && p.tiltY()==0);
        bytes[1]=0;check(!new Packet(bytes).batteryValid);
        boolean rejected=false;
        try {new Packet(new byte[19]);}catch(IllegalArgumentException expected){rejected=true;}
        check(rejected);bytes[0]=2;rejected=false;
        try {new Packet(bytes);}catch(IllegalArgumentException expected){rejected=true;}
        check(rejected);
        System.out.println("Packet tests passed: signed fields, uptime wrap, battery validity, angles, malformed/version rejection.");
    }
}