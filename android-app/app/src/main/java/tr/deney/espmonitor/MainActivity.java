package tr.deney.espmonitor;

import android.Manifest;
import android.app.*;
import android.bluetooth.*;
import android.bluetooth.le.*;
import android.content.*;
import android.content.pm.PackageManager;
import android.graphics.*;
import android.graphics.drawable.GradientDrawable;
import android.os.*;
import android.view.*;
import android.widget.*;
import java.util.*;

public class MainActivity extends Activity {
    static final UUID SERVICE=UUID.fromString("87ba0001-6d31-4b6e-8bd0-ef38247a9100");
    static final UUID DATA=UUID.fromString("87ba0002-6d31-4b6e-8bd0-ef38247a9100");
    static final UUID SENSORS=UUID.fromString("87ba0003-6d31-4b6e-8bd0-ef38247a9100");
    static final UUID COMMAND=UUID.fromString("87ba0004-6d31-4b6e-8bd0-ef38247a9100");
    static final UUID CCCD=UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    // Karttaki ble_service.cpp CMD_* ile ayni
    static final byte CMD_SET_TIME=1, CMD_LOG_NOW=2, CMD_BRIGHTNESS=3, CMD_LOGGING=4;
    // Karttaki APP_DATA_FRESH_SECONDS / APP_DATA_STALE_SECONDS ile ayni
    static final int FRESH_S=15, STALE_S=60;
    static final int BG=Color.rgb(11,16,36), CARD=Color.rgb(23,36,69), BORDER=Color.rgb(37,48,79),
        TEXT=Color.rgb(234,240,255), DIM=Color.rgb(138,150,184), ACCENT=Color.rgb(34,211,238),
        GOOD=Color.rgb(52,211,153), WARN=Color.rgb(251,191,36), BAD=Color.rgb(248,113,113);

    final Handler handler=new Handler(Looper.getMainLooper());
    final Map<String,BluetoothDevice> devices=new LinkedHashMap<>();
    final ArrayDeque<Runnable> ops=new ArrayDeque<>();
    boolean opBusy=false;
    BluetoothAdapter adapter;
    BluetoothLeScanner scanner;
    BluetoothGatt gatt;
    BluetoothGattCharacteristic commandChar;
    SharedPreferences prefs;
    TextView status, values, angles, statusLine, logLine, demoNote;
    TextView[] sensorValue=new TextView[5], sensorNote=new TextView[5];
    LinearLayout deviceList, controls;
    Button logToggle;
    SeekBar brightness;
    boolean draggingBrightness=false;
    long lastBrightnessSend=0;
    LevelView level;
    boolean scanning=false, active=false, userClosed=false, timeSynced=false;
    long lastData=0, lastSensor=0;
    float zeroX=0,zeroY=0;
    Packet latest;
    SensorPacket latestSensors;
    final Runnable scanTimeout=()->stopScan("Arama bitti. Listeden kartınızı seçin.");
    final Runnable connectTimeout=()->{if(gatt!=null && lastData==0 && lastSensor==0) lost("Bağlantı zaman aşımı.");};
    final Runnable reconnect=()->connectSaved();
    final Runnable freshness=new Runnable() {
        public void run() {
            if(!active) return;
            long now=SystemClock.elapsedRealtime();
            if(gatt!=null && lastSensor>0 && now-lastSensor>3000) {
                status.setText("Veri bekleniyor — son değerler güncel değil");
                level.live=false; level.invalidate();
            }
            handler.postDelayed(this,1000);
        }
    };

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        prefs=getSharedPreferences("kart",MODE_PRIVATE);
        BluetoothManager manager=getSystemService(BluetoothManager.class);
        adapter=manager==null?null:manager.getAdapter();
        LinearLayout root=new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(BG);
        int pad=dp(16); root.setPadding(pad,pad,pad,pad);
        root.setOnApplyWindowInsetsListener((v,insets)->{
            root.setPadding(pad,pad+insets.getSystemWindowInsetTop(),pad,pad+insets.getSystemWindowInsetBottom());
            return insets;
        });
        ScrollView scroll=new ScrollView(this); scroll.setBackgroundColor(BG); scroll.addView(root); setContentView(scroll);

        text(root,"DENEY KARTI",26,TEXT).setTypeface(Typeface.DEFAULT_BOLD);
        status=text(root,"Başlamak için Kartı bul düğmesine dokunun.",16,DIM);
        LinearLayout row=hrow(root);
        Button scan=button(row,"Kartı bul"); scan.setOnClickListener(v->prepareScan());
        Button end=button(row,"Bağlantıyı kes");
        end.setOnClickListener(v->{userClosed=true;handler.removeCallbacks(reconnect);stopScan(null);disconnect("Bağlantı kapatıldı.");});
        deviceList=new LinearLayout(this); deviceList.setOrientation(LinearLayout.VERTICAL); root.addView(deviceList);

        section(root,"Sensörler");
        demoNote=text(root,"",14,WARN); demoNote.setVisibility(View.GONE);
        String[][] cards={{"Sıcaklık","°C"},{"Nem","%"},{"Gerilim","V"},{"Akım","A"},{"Güç","W"}};
        LinearLayout grid=null;
        for(int i=0;i<cards.length;i++){
            if(i%2==0) grid=hrow(root);
            sensorCard(grid,i,cards[i][0],cards[i][1]);
        }
        if(cards.length%2==1) grid.addView(new View(this),new LinearLayout.LayoutParams(0,1,1));

        section(root,"Kart durumu");
        statusLine=text(root,"—",15,TEXT);
        logLine=text(root,"",15,DIM);

        section(root,"Kartı yönet");
        controls=new LinearLayout(this); controls.setOrientation(LinearLayout.VERTICAL); root.addView(controls);
        LinearLayout c1=hrow(controls);
        button(c1,"Saati telefondan ayarla").setOnClickListener(v->sendTime(true));
        button(c1,"Şimdi kaydet").setOnClickListener(v->{if(sendCommand(new byte[]{CMD_LOG_NOW}))toast("SD karta kayıt istendi");});
        logToggle=button(controls,"Kaydı başlat");
        logToggle.setOnClickListener(v->{
            if(latestSensors==null)return;
            boolean on=!latestSensors.logging;
            if(sendCommand(new byte[]{CMD_LOGGING,(byte)(on?1:0)}))toast(on?"Kayıt başlatılıyor":"Kayıt durduruluyor");
        });
        text(controls,"Ekran parlaklığı",14,DIM);
        brightness=new SeekBar(this); brightness.setMax(100); controls.addView(brightness);
        brightness.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar s,int p,boolean user){
                long now=SystemClock.elapsedRealtime();
                if(user && now-lastBrightnessSend>250){lastBrightnessSend=now;sendCommand(new byte[]{CMD_BRIGHTNESS,(byte)p});}
            }
            public void onStartTrackingTouch(SeekBar s){draggingBrightness=true;}
            public void onStopTrackingTouch(SeekBar s){draggingBrightness=false;sendCommand(new byte[]{CMD_BRIGHTNESS,(byte)s.getProgress()});}
        });
        setControlsEnabled(false);

        section(root,"Eğim (su terazisi)");
        level=new LevelView(this); root.addView(level,new LinearLayout.LayoutParams(-1,dp(240)));
        angles=text(root,"X: —     Y: —",24,TEXT);
        values=text(root,"İvme, dönüş hızı ve pil verileri burada görünecek.",15,DIM);
        Button zero=button(root,"Bu konumu sıfır kabul et");
        zero.setOnClickListener(v->{
            if(latest!=null && SystemClock.elapsedRealtime()-lastData<3000) {
                zeroX=latest.tiltX(); zeroY=latest.tiltY(); render(latest);
            } else status.setText("Sıfırlamak için canlı veri bağlantısı gerekiyor.");
        });
        text(root,"Veriler karttan Bluetooth ile gelir; internet gerekmez. Uygulama açıkken son bağlanılan karta otomatik bağlanılır.",13,DIM);
        clearValues();
    }

    // ---- görünüm yardımcıları ----
    int dp(int n){return (int)(n*getResources().getDisplayMetrics().density);}
    TextView text(LinearLayout root,String s,int size,int color){
        TextView t=new TextView(this); t.setText(s);t.setTextSize(size);t.setTextColor(color);
        t.setPadding(0,dp(4),0,dp(4));root.addView(t);return t;
    }
    void section(LinearLayout root,String title){
        TextView t=text(root,title.toUpperCase(new Locale("tr")),13,ACCENT);
        t.setPadding(0,dp(18),0,dp(6)); t.setLetterSpacing(0.08f);
    }
    LinearLayout hrow(LinearLayout root){
        LinearLayout r=new LinearLayout(this); r.setOrientation(LinearLayout.HORIZONTAL); root.addView(r); return r;
    }
    Button button(LinearLayout parent,String label){
        Button b=new Button(this); b.setText(label); b.setAllCaps(false);
        if(parent.getOrientation()==LinearLayout.HORIZONTAL) parent.addView(b,new LinearLayout.LayoutParams(0,-2,1));
        else parent.addView(b);
        return b;
    }
    void sensorCard(LinearLayout row,int i,String label,String unit){
        LinearLayout card=new LinearLayout(this); card.setOrientation(LinearLayout.VERTICAL);
        GradientDrawable bg=new GradientDrawable(); bg.setColor(CARD); bg.setCornerRadius(dp(14)); bg.setStroke(dp(1),BORDER);
        card.setBackground(bg); card.setPadding(dp(14),dp(10),dp(14),dp(10));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-2,1); lp.setMargins(dp(4),dp(4),dp(4),dp(4));
        row.addView(card,lp);
        text(card,label+" ("+unit+")",13,DIM);
        sensorValue[i]=text(card,"—",28,TEXT); sensorValue[i].setTypeface(Typeface.MONOSPACE);
        sensorNote[i]=text(card,"",12,WARN);
    }
    void toast(String s){Toast.makeText(this,s,Toast.LENGTH_SHORT).show();}
    void setControlsEnabled(boolean on){
        for(int i=0;i<controls.getChildCount();i++){
            View v=controls.getChildAt(i); v.setEnabled(on); v.setAlpha(on?1f:0.4f);
            if(v instanceof LinearLayout) for(int j=0;j<((LinearLayout)v).getChildCount();j++) ((LinearLayout)v).getChildAt(j).setEnabled(on);
        }
    }

    // ---- izin / tarama ----
    boolean permissions(){
        if(Build.VERSION.SDK_INT>=31) return checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN)==PackageManager.PERMISSION_GRANTED
            && checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)==PackageManager.PERMISSION_GRANTED;
        return checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION)==PackageManager.PERMISSION_GRANTED;
    }
    void prepareScan(){
        if(adapter==null){status.setText("Bu telefonda Bluetooth bulunamadı.");return;}
        if(!permissions()){
            requestPermissions(Build.VERSION.SDK_INT>=31
                ?new String[]{Manifest.permission.BLUETOOTH_SCAN,Manifest.permission.BLUETOOTH_CONNECT}
                :new String[]{Manifest.permission.ACCESS_FINE_LOCATION,Manifest.permission.ACCESS_COARSE_LOCATION},10);return;
        }
        if(!adapter.isEnabled()){
            try { startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE),11); }
            catch(SecurityException e) { status.setText("Bluetooth izni gerekiyor."); }
            return;
        }
        userClosed=true; handler.removeCallbacks(reconnect);
        stopScan(null);disconnect(null);devices.clear();deviceList.removeAllViews();
        scanner=adapter.getBluetoothLeScanner();
        if(scanner==null){status.setText("Bluetooth tarayıcı başlatılamadı.");return;}
        scanning=true;
        status.setText("Kart aranıyor… (15 saniye)");
        try {
            scanner.startScan(Collections.singletonList(new ScanFilter.Builder().setServiceUuid(new ParcelUuid(SERVICE)).build()),
                new ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(),scanCallback);
            handler.postDelayed(scanTimeout,15000);
        } catch(SecurityException|IllegalStateException e){stopScan("Bluetooth iznini ve Bluetooth durumunu kontrol edin.");}
    }
    void stopScan(String message){
        handler.removeCallbacks(scanTimeout);
        if(scanning && scanner!=null)try{scanner.stopScan(scanCallback);}catch(SecurityException|IllegalStateException ignored){}
        scanning=false;if(message!=null)status.setText(devices.isEmpty()?message+" Kart yoksa gücünü kontrol edin. Android 8–11'de Konum açık olmalı.":message);
    }
    final ScanCallback scanCallback=new ScanCallback(){
        @Override public void onScanResult(int type,ScanResult result){
            handler.post(()->{
                if(!scanning||!active)return;
                BluetoothDevice d=result.getDevice();
                String address=d.getAddress();
                if(devices.containsKey(address))return;
                devices.put(address,d);
                Button b=new Button(MainActivity.this); b.setAllCaps(false);
                b.setText("Deney-Karti • "+address);
                deviceList.addView(b);b.setOnClickListener(v->connect(d));
            });
        }
        @Override public void onScanFailed(int code){handler.post(()->stopScan("Tarama hatası: "+code));}
    };

    // ---- bağlantı ----
    void connectSaved(){
        String address=prefs.getString("address",null);
        if(!active||address==null||gatt!=null||adapter==null||!permissions()||!adapter.isEnabled())return;
        try{connect(adapter.getRemoteDevice(address));}catch(IllegalArgumentException ignored){}
    }
    void connect(BluetoothDevice d){
        stopScan(null);disconnect(null);deviceList.removeAllViews();
        lastData=lastSensor=0;zeroX=zeroY=0;userClosed=false;timeSynced=false;
        prefs.edit().putString("address",d.getAddress()).apply();
        status.setText("Karta bağlanılıyor…");
        try {gatt=d.connectGatt(this,false,callback,BluetoothDevice.TRANSPORT_LE);
            handler.postDelayed(connectTimeout,20000);
        }catch(SecurityException e){disconnect("Yakındaki cihazlar izni gerekiyor.");}
    }
    // Beklenmeyen kopma: kullanıcı kapatmadıysa 3 sn sonra yeniden dene.
    void lost(String message){
        disconnect(userClosed?message:message+" Yeniden bağlanılıyor…");
        if(!userClosed && active){handler.removeCallbacks(reconnect);handler.postDelayed(reconnect,3000);}
    }
    void disconnect(String message){
        handler.removeCallbacks(connectTimeout);
        ops.clear();opBusy=false;commandChar=null;
        BluetoothGatt old=gatt;gatt=null;
        if(old!=null){try{old.disconnect();}catch(SecurityException ignored){}try{old.close();}catch(SecurityException ignored){}}
        lastData=lastSensor=0;latest=null;latestSensors=null;level.live=false;level.invalidate();
        clearValues();
        if(message!=null)status.setText(message);
    }
    void clearValues(){
        angles.setText("X: —     Y: —");values.setText("Canlı veri bağlantısı yok.");
        for(int i=0;i<5;i++){sensorValue[i].setText("—");sensorValue[i].setTextColor(DIM);sensorNote[i].setText("");}
        statusLine.setText("Kart bağlı değil.");logLine.setText("");demoNote.setVisibility(View.GONE);
        setControlsEnabled(false);
    }

    // GATT işlemleri Android'de tek tek yapılmalı; kuyruk bir önceki bitince sıradakini başlatır.
    void enqueue(Runnable op){ops.add(op);if(!opBusy)nextOp();}
    void nextOp(){Runnable r=ops.poll();opBusy=r!=null;if(r!=null)r.run();}
    void opDone(){opBusy=false;nextOp();}

    @SuppressWarnings("deprecation")
    void subscribe(BluetoothGattCharacteristic c){
        enqueue(()->{
            BluetoothGatt g=gatt;
            BluetoothGattDescriptor d=c.getDescriptor(CCCD);
            boolean ok=false;
            try{
                if(g!=null&&d!=null&&g.setCharacteristicNotification(c,true)){
                    if(Build.VERSION.SDK_INT>=33) ok=g.writeDescriptor(d,BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)==BluetoothStatusCodes.SUCCESS;
                    else{d.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);ok=g.writeDescriptor(d);}
                }
            }catch(SecurityException ignored){}
            if(!ok){lost("Veri aboneliği başlatılamadı.");}
        });
    }
    @SuppressWarnings("deprecation")
    boolean sendCommand(byte[] value){
        BluetoothGattCharacteristic c=commandChar;
        if(gatt==null||c==null){toast("Önce karta bağlanın");return false;}
        enqueue(()->{
            BluetoothGatt g=gatt;
            boolean ok=false;
            try{
                if(g!=null){
                    if(Build.VERSION.SDK_INT>=33) ok=g.writeCharacteristic(c,value,BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)==BluetoothStatusCodes.SUCCESS;
                    else{c.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);c.setValue(value);ok=g.writeCharacteristic(c);}
                }
            }catch(SecurityException ignored){}
            if(!ok){toast("Komut gönderilemedi");handler.post(this::opDone);}
        });
        return true;
    }
    void sendTime(boolean manual){
        Calendar c=Calendar.getInstance();
        int y=c.get(Calendar.YEAR);
        byte[] v={CMD_SET_TIME,(byte)(y&255),(byte)(y>>8),(byte)(c.get(Calendar.MONTH)+1),(byte)c.get(Calendar.DAY_OF_MONTH),
            (byte)c.get(Calendar.HOUR_OF_DAY),(byte)c.get(Calendar.MINUTE),(byte)c.get(Calendar.SECOND),(byte)(c.get(Calendar.DAY_OF_WEEK)-1)};
        if(sendCommand(v)){timeSynced=true;if(manual)toast("Kartın saati telefona göre ayarlandı");}
    }

    final BluetoothGattCallback callback=new BluetoothGattCallback(){
        @Override public void onConnectionStateChange(BluetoothGatt g,int code,int state){
            handler.post(()->{
                if(g!=gatt)return;
                if(code!=BluetoothGatt.GATT_SUCCESS||state==BluetoothProfile.STATE_DISCONNECTED){
                    lost("Bağlantı kesildi.");return;
                }
                if(state==BluetoothProfile.STATE_CONNECTED){
                    status.setText("Sensör servisi hazırlanıyor…");
                    try{if(!g.discoverServices())lost("Servis araması başlatılamadı.");}
                    catch(SecurityException e){disconnect("Bluetooth izni kaldırıldı.");}
                }
            });
        }
        @Override public void onServicesDiscovered(BluetoothGatt g,int code){
            handler.post(()->{
                if(g!=gatt)return;
                BluetoothGattService service=g.getService(SERVICE);
                if(code!=BluetoothGatt.GATT_SUCCESS||service==null){userClosed=true;disconnect("Kart yazılımı uyumlu değil.");return;}
                BluetoothGattCharacteristic sensors=service.getCharacteristic(SENSORS);
                BluetoothGattCharacteristic motion=service.getCharacteristic(DATA);
                commandChar=service.getCharacteristic(COMMAND);
                if(sensors==null) status.setText("Kart yazılımı eski: sensör verisi için kartı güncelleyin.");
                else subscribe(sensors);
                if(motion!=null) subscribe(motion);
            });
        }
        @Override public void onDescriptorWrite(BluetoothGatt g,BluetoothGattDescriptor d,int code){
            handler.post(()->{if(g==gatt){
                if(code!=BluetoothGatt.GATT_SUCCESS){lost("Veri aboneliği başarısız.");return;}
                if(lastSensor==0)status.setText("Bağlandı • ilk ölçüm bekleniyor…");
                opDone();
            }});
        }
        @Override public void onCharacteristicWrite(BluetoothGatt g,BluetoothGattCharacteristic c,int code){
            handler.post(()->{if(g==gatt){
                if(code!=BluetoothGatt.GATT_SUCCESS)toast("Kart komutu reddetti");
                opDone();
            }});
        }
        @Override public void onCharacteristicChanged(BluetoothGatt g,BluetoothGattCharacteristic c){
            receive(g,c.getUuid(),c.getValue().clone());
        }
        @Override public void onCharacteristicChanged(BluetoothGatt g,BluetoothGattCharacteristic c,byte[] value){
            receive(g,c.getUuid(),value.clone());
        }
    };
    void receive(BluetoothGatt g,UUID uuid,byte[] bytes){
        handler.post(()->{
            if(g!=gatt||!active)return;
            try{
                if(DATA.equals(uuid)){
                    Packet p=new Packet(bytes);latest=p;lastData=SystemClock.elapsedRealtime();render(p);
                }else if(SENSORS.equals(uuid)){
                    SensorPacket s=new SensorPacket(bytes);latestSensors=s;lastSensor=SystemClock.elapsedRealtime();render(s);
                    // Kartın saati geçersizse (ör. RTC pili bitmiş) bir kez telefonun saatini gönder.
                    if(!s.rtc&&!timeSynced)sendTime(false);
                }else return;
                handler.removeCallbacks(connectTimeout);
                status.setText("Bağlandı • canlı veri");
            }catch(IllegalArgumentException e){userClosed=true;disconnect("Desteklenmeyen veri biçimi. Kart ve uygulama sürümlerini eşleyin.");}
        });
    }
    void render(SensorPacket s){
        setControlsEnabled(commandChar!=null);
        float[] v={s.temperature,s.humidity,s.voltage,s.current,s.power};
        boolean[] ok={s.temperatureValid,s.humidityValid,s.voltageValid,s.currentValid,s.powerValid};
        String[] fmt={"%.1f","%.1f","%.2f","%.2f","%.1f"};
        for(int i=0;i<5;i++){
            if(!ok[i]){sensorValue[i].setText("—");sensorValue[i].setTextColor(DIM);sensorNote[i].setText("veri yok");continue;}
            sensorValue[i].setText(String.format(Locale.getDefault(),fmt[i],v[i]));
            sensorValue[i].setTextColor(s.ageSeconds<FRESH_S?TEXT:WARN);
            sensorNote[i].setText(s.ageSeconds>=STALE_S?"ESKİ VERİ":"");
        }
        demoNote.setText("DEMO: MQTT verisi gelmediği için kart örnek değerler üretiyor.");
        demoNote.setVisibility(s.demo?View.VISIBLE:View.GONE);
        statusLine.setText(String.format(Locale.getDefault(),"Wi-Fi %s   •   MQTT %s   •   SD %s\nSaat %s   •   Pil %s",
            s.wifi?"✓":"✗",s.mqtt?"✓":"✗",s.sd?"✓":"✗",s.rtc?"✓":"geçersiz",
            s.batteryValid?s.batteryPercent+"%":"yok"));
        logLine.setText(s.logging
            ?String.format(Locale.getDefault(),"SD kayıt açık • %d satır%s",s.logRows,s.logRows>0&&!s.lastLogOk?" • son yazma başarısız":"")
            :"SD kayıt kapalı");
        logToggle.setText(s.logging?"Kaydı durdur":"Kaydı başlat");
        if(!draggingBrightness)brightness.setProgress(s.brightness);
    }
    void render(Packet p){
        float x=p.tiltX()-zeroX,y=p.tiltY()-zeroY;
        level.x=x;level.y=y;level.live=true;level.invalidate();
        angles.setText(String.format(Locale.getDefault(),"X: %.1f°    Y: %.1f°",x,y));
        values.setText(String.format(Locale.getDefault(),
            "İvme (g)  X %.3f   Y %.3f   Z %.3f\nDönüş hızı (°/s)  X %.2f   Y %.2f   Z %.2f\nPil: %s   •   Çalışma süresi: %.0f s",
            p.x,p.y,p.z,p.gx,p.gy,p.gz,p.batteryValid?String.format(Locale.getDefault(),"%.2f V",p.voltage):"ölçüm yok",p.uptime/1000.0));
    }
    @Override public void onRequestPermissionsResult(int request,String[] names,int[] results){
        super.onRequestPermissionsResult(request,names,results);
        if(request==10){if(permissions())prepareScan();else status.setText("Bağlanmak için Bluetooth/konum iznini uygulama ayarlarından açın.");}
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(request==11 && result==RESULT_OK)prepareScan();
    }
    @Override protected void onStart(){
        super.onStart();active=true;handler.post(freshness);
        if(prefs.getString("address",null)!=null && permissions() && adapter!=null && adapter.isEnabled()){
            userClosed=false;connectSaved();
        }
    }
    @Override protected void onStop(){
        active=false;handler.removeCallbacks(reconnect);stopScan(null);disconnect("Devam etmek için Kartı bul düğmesine dokunun.");
        handler.removeCallbacks(freshness);super.onStop();
    }
    class LevelView extends View{
        final Paint paint=new Paint(3);float x,y;boolean live=false;
        LevelView(Context c){super(c);setContentDescription("Kartın eğimini gösteren su terazisi");}
        @Override protected void onDraw(Canvas c){
            float cx=getWidth()/2f,cy=getHeight()/2f,r=Math.min(cx,cy)-dp(12);
            paint.setStyle(Paint.Style.FILL);paint.setColor(CARD);c.drawCircle(cx,cy,r,paint);
            paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(dp(2));paint.setColor(Color.rgb(84,105,145));
            c.drawCircle(cx,cy,r,paint);c.drawCircle(cx,cy,r/3,paint);
            c.drawLine(cx-r,cy,cx+r,cy,paint);c.drawLine(cx,cy-r,cx,cy+r,paint);
            float bx=x/30,by=y/30,len=(float)Math.hypot(bx,by);
            if(len>1){bx/=len;by/=len;}
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(!live?Color.GRAY:Math.abs(x)<1&&Math.abs(y)<1?GOOD:ACCENT);
            c.drawCircle(cx+bx*(r-dp(22)),cy+by*(r-dp(22)),dp(20),paint);
        }
    }
}
