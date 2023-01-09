package com.example.bme280;


import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.icu.text.SimpleDateFormat;
import android.icu.util.Calendar;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    //****** GUI Components ******
    // switch buttons
    Button startScanningButton;
    Button stopScanningButton;
    Button disconnectDevice;
    Button setAltitude;
    Button setTemperature;
    Button sendnow;
    Button exitnow;

    TextView DispTemp;
    TextView DispHum;
    TextView DispAlt;
    TextView DispPres;
    TextView statusview;
    TextView updateTime;

    // bluetooth variables
    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    BluetoothGatt bluetoothGatt;

    final String BME280_SERVICE = "19B10010-E8F2-537E-4F6C-D104768A1214";
    final String BME280_RX = "19B10011-E8F2-537E-4F6C-D104768A1214";        // TX for central
    final String BME280_TX = "19B10012-E8F2-537E-4F6C-D104768A1214";        // RX for central (notify)
    final String CONFIG_DESCRIPTOR = "00002902-0000-1000-8000-00805f9b34fb";
    final String DEVICENAME = "Peripheral BME280 BLE";

    //private final static int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;

    Boolean btScanning = false;
    BluetoothDevice DeviceDiscovered = null;

    // Stops scanning after 5 seconds.
    private final Handler mHandler = new Handler();
    private static final long SCAN_PERIOD = 5000;

    ActivityResultLauncher<Intent> someActivityResultLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                if (result.getResultCode() == Activity.RESULT_OK) {
                    // There are no request codes
                    System.out.println("Bluetooth access enabled");
                    //Intent data = result.getData();
//                                doSomeOperations();
                }
            });

    // commands available for sketch
    private static final int REQUEST_METERS  = '1';
    private static final int REQUEST_FEET    = '2';
    private static final int REQUEST_CELSIUS = '3';
    private static final int REQUEST_FRHEIT  = '4';
    //private static final int REQUEST_STOP    = '5';
    //private static final int REQUEST_START   = '6';
    private static final int REQUEST_NOW     = '7';

    int CurrentlyMeters = 1;                     // real status will be set on receive
    int CurrentlyCelsius = 1;                    // real status will be set on receive
    boolean SetStartScanningButton = true;       // if StopScanning is called it will make the scan button VISIBLE

    // handle BME280 message
    int TotalLength = 0;                         // total length (maybe split on multiple messages
    int[] TotalMessage = new int[50];            // holds the total message
    int MessageOffset = 0;                       // current offset in TotalMessage
    private static final int MAGICNUM = -49;     // indicate start of NEW message (0xcf)

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRequestedOrientation (ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        setContentView(R.layout.activity_main);

        // needed to display results
        DispTemp = findViewById(R.id.Res_temp);
        DispHum =  findViewById(R.id.Res_hum);
        DispAlt =  findViewById(R.id.Res_alt);
        DispPres = findViewById(R.id.Res_press);
        statusview = findViewById(R.id.Status_view);
        updateTime = findViewById(R.id.LastUpdate);

        startScanningButton = findViewById(R.id.Start_scan);
        startScanningButton.setOnClickListener(this::startScanning);
        startScanningButton.setVisibility(View.VISIBLE);

        stopScanningButton =  findViewById(R.id.Stop_scan);
        stopScanningButton.setOnClickListener(v -> stopScanning());
        stopScanningButton.setVisibility(View.INVISIBLE);

        disconnectDevice =  findViewById(R.id.Disconnect_button);
        disconnectDevice.setOnClickListener(this::disconnect);
        disconnectDevice.setVisibility(View.INVISIBLE);

        setTemperature =  findViewById(R.id.Button_Temp_Sel);
        setTemperature.setOnClickListener(this::SwitchTemp);
        setTemperature.setVisibility(View.INVISIBLE);

        setAltitude = findViewById(R.id.Button_Alt_Sel);
        setAltitude.setOnClickListener(this::SwitchAltitude);
        setAltitude.setVisibility(View.INVISIBLE);

        sendnow = findViewById(R.id.Button_Send_Now);
        sendnow.setOnClickListener(this::ReqSendNow);
        sendnow.setVisibility(View.INVISIBLE);

        exitnow = findViewById(R.id.Exitnow);
        exitnow.setOnClickListener(this::exitAppCLICK);
        exitnow.setVisibility(View.VISIBLE);

        SetBME280Fields(false);

        btManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btManager.getAdapter();
        btScanner = btAdapter.getBluetoothLeScanner();

        if (btAdapter != null && !btAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            //startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
            someActivityResultLauncher.launch(enableIntent);
        }

        // Make sure we have access coarse location enabled, if not, prompt the user to enable it
        if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            final AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("This app needs location access");
            builder.setMessage("Please grant location access so this app can detect peripherals.");
            builder.setPositiveButton(android.R.string.ok, null);
            builder.setOnDismissListener(dialog -> requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION));
            builder.show();
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case PERMISSION_REQUEST_COARSE_LOCATION: {
                if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    System.out.println("coarse location permission granted");
                } else {
                    final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("Functionality limited");
                    builder.setMessage("Since location access has not been granted, this app will not be able to discover beacons when in the background.");
                    builder.setPositiveButton(android.R.string.ok, null);
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {

                        @Override
                        public void onDismiss(DialogInterface dialog) {
                        }

                    });
                    builder.show();
                }
            }
        }
    }

    public void updateButtonTemp(){
        if (CurrentlyCelsius == 1) setTemperature.setText(R.string.Fahrenheit);
        else setTemperature.setText(R.string.Celsius);
    }

    public void updateButtonAlt(){
        if (CurrentlyMeters == 1) setAltitude.setText(R.string.Feet);
        else setAltitude.setText(R.string.Meter);
    }

    // request for sending BME280 now
    public void ReqSendNow(View v){
        writeCommand(REQUEST_NOW);
    }

    // switch between meters and feet
    public void SwitchAltitude(View v) {
        if (CurrentlyMeters == 1) {
            writeCommand(REQUEST_FEET);
            CurrentlyMeters = 0;
        }
        else {
            writeCommand(REQUEST_METERS);
            CurrentlyMeters = 1;
        }

        updateButtonAlt();
    }

    // switch between celsius and fahrenheit
    public void SwitchTemp(View v) {
        if (CurrentlyCelsius == 1) {
            writeCommand(REQUEST_FRHEIT);
            CurrentlyCelsius = 0;
        }
        else {
            writeCommand(REQUEST_CELSIUS);
            CurrentlyCelsius = 1;
       }
        updateButtonTemp();
    }

    // show or hide BME280 result fields
    public void SetBME280Fields(boolean cmd){
        MainActivity.this.runOnUiThread(() -> {
            if(cmd) {
                DispTemp.setVisibility((View.VISIBLE));
                DispAlt.setVisibility((View.VISIBLE));
                DispHum.setVisibility((View.VISIBLE));
                DispPres.setVisibility((View.VISIBLE));
                updateTime.setVisibility((View.VISIBLE));
            }
            else {
                DispTemp.setVisibility((View.INVISIBLE));
                DispAlt.setVisibility((View.INVISIBLE));
                DispHum.setVisibility((View.INVISIBLE));
                DispPres.setVisibility((View.INVISIBLE));
                updateTime.setVisibility((View.INVISIBLE));
            }
        });
    }

    // disconnect and reset
    @SuppressLint("MissingPermission")
    public void disconnect(View view) {
        // called for disconnect button
        NotificationControl(false);
        setAltitude.setVisibility(View.INVISIBLE);
        setTemperature.setVisibility(View.INVISIBLE);
        sendnow.setVisibility(View.INVISIBLE);
        SetStartScanningButton = true;
        startScanningButton.setVisibility(View.VISIBLE);
        exitnow.setVisibility(View.VISIBLE);
        disconnectDevice.setVisibility(View.INVISIBLE);
        SetBME280Fields(false);
        bluetoothGatt.disconnect();

        Toast.makeText(getApplicationContext(),getString(R.string.discon),Toast.LENGTH_SHORT).show();
    }

    // start to scan for device
    public void startScanning(View v) {
        System.out.println("start scanning");
        btScanning = true;
        SetStartScanningButton = true;
        startScanningButton.setVisibility(View.INVISIBLE);
        stopScanningButton.setVisibility(View.VISIBLE);
        AsyncTask.execute(new Runnable() {
            @SuppressLint("MissingPermission")
            @Override
            public void run() {
               btScanner.startScan(leScanCallback);
            }
        });

        // for only a short time
        mHandler.postDelayed(this::stopScanning, SCAN_PERIOD);

        Toast.makeText(getApplicationContext(),getString(R.string.scanstart),Toast.LENGTH_SHORT).show();
    }

    // stop scanning
    public void stopScanning() {

        if (SetStartScanningButton) {
            startScanningButton.setVisibility(View.VISIBLE);
            exitnow.setVisibility(View.VISIBLE);
        }
        stopScanningButton.setVisibility(View.INVISIBLE);

        // if still scanning
        if (btScanning) {
            AsyncTask.execute(new Runnable() {
                @SuppressLint("MissingPermission")
                @Override
                public void run() {
                    btScanner.stopScan(leScanCallback);
                }
            });

            Toast.makeText(getApplicationContext(), getString(R.string.scanstop), Toast.LENGTH_SHORT).show();
            btScanning = false;
        }
    }

    // Device scan callback.
    private final ScanCallback leScanCallback = new ScanCallback() {
        @SuppressLint("MissingPermission")
        @Override
        public void onScanResult(int callbackType, ScanResult result) {

            if (result.getDevice().getName() != null) {
                if (result.getDevice().getName().equalsIgnoreCase(DEVICENAME)) {
                    updateStatusView(R.string.DeviceDetected, R.color.green, 0);
                    DeviceDiscovered= result.getDevice();
                    //System.out.println("Found device");
                    SetStartScanningButton = false;
                    stopScanning();
                    connectToDevice();
                }
            }
        }
    };

    /*
    update statusview
    m = pointer to R.String.xxxx
    color = R.color.xxx
    delay = mS after which to clear and return to default colors (0 = is do not restore)
     */
    public void updateStatusView(int m, int color, int delay) {

        MainActivity.this.runOnUiThread(new Runnable() {
            public void run() {
                statusview.setText(getString(m));
                statusview.setBackgroundResource(color);
            }
        });

        if (delay > 0){
            mHandler.postDelayed(this::ResetStatusBackground, delay);
        }
    }

    // restore to default color and clear text
    public void ResetStatusBackground()
    {
        MainActivity.this.runOnUiThread(() -> {
            statusview.setText("");
            statusview.setBackgroundResource(R.color.background);
        });
    }

    // write command to the characteristic
    @SuppressLint("MissingPermission")
    public boolean writeCommand(int Command) {

        BluetoothGattCharacteristic Character = null;

        if (bluetoothGatt != null) {
            Character = bluetoothGatt.getService(UUID.fromString(BME280_SERVICE)).getCharacteristic(UUID.fromString(BME280_RX));
        }

        if (Character == null) {
            updateStatusView(R.string.SendCharNotFound, R.color.red, 5000);
            return false;
        }

        Character.setValue(Command, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        return (bluetoothGatt.writeCharacteristic(Character));
    }

    // connect to device
    @SuppressLint("MissingPermission")
    public void connectToDevice() {
        updateStatusView(R.string.TryConnect, R.color.green, 0);
        bluetoothGatt = DeviceDiscovered.connectGatt(this, false, btleGattCallback);
    }

    @SuppressLint("MissingPermission")
    public boolean NotificationControl(boolean enable) {
        // enable / disable notifications
        BluetoothGattCharacteristic Character = null;

        if (bluetoothGatt != null) {
            Character = bluetoothGatt.getService(UUID.fromString(BME280_SERVICE)).getCharacteristic(UUID.fromString(BME280_TX));
        }

        if (Character == null) {
            //System.out.println("Notify characteristic not found!");
            updateStatusView(R.string.NotifyError, R.color.red, 3000);
            return false;
        }

        //bluetoothGatt.setCharacteristicNotification(Character, true);

        BluetoothGattDescriptor desc = Character.getDescriptor(UUID.fromString(CONFIG_DESCRIPTOR));

        if (desc == null) {
            updateStatusView(R.string.CCDError, R.color.red, 3000);
            return false;
        }

        // enable or disable
        if (enable){
            bluetoothGatt.setCharacteristicNotification(Character, true);
            desc.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE );
            updateStatusView(R.string.NotifyEnabled, R.color.green, 3000);
        }
        else {
            desc.setValue(new byte[] { 0x00, 0x00 });
            bluetoothGatt.setCharacteristicNotification(Character, false);
            updateStatusView(R.string.NotifyDisabled, R.color.orange, 3000);
        }
        bluetoothGatt.writeDescriptor(desc);
        return true;
    }

    // display the received BME280 information
    /* format data in the message is

    offset
    0     4 bytes  : float humidity;
    4     4 bytes  : float pressure;
    8     4 bytes  : float altitude;
    12    4 bytes  : float temperature;
    16    1 byte   : byte meter;       // if 1 altitude is in meter, else in feet
    17    1 byte   : byte celsius;     // if 1 temperature is celsius, else fahrenheit

    total 18 bytes
    */

    public void display280() {
        // display the results received (called from notification)
        final float humfl = bytetofloat(0);
        final float pressfl = bytetofloat(4);
        final float altfl = bytetofloat(8);
        final float tempfl = bytetofloat(12);

        // get the meter / feet inf
        CurrentlyMeters = TotalMessage[16];

        // get the celsius / Fahrenheit
        CurrentlyCelsius = TotalMessage[17];

        SetBME280Fields(true);

        MainActivity.this.runOnUiThread(new Runnable() {
            public void run() {

                updateButtonAlt();
                updateButtonTemp();

                String DispText;

                Calendar c = Calendar.getInstance();
                SimpleDateFormat df = new SimpleDateFormat("HH:mm:ss");
                String formattedDate = df.format(c.getTime());
                DispText = getString(R.string.UpdateTime, formattedDate);
                updateTime.setText(DispText);

                if (CurrentlyCelsius== 1) DispText = getString(R.string.res_temp_txt_C, tempfl);
                else DispText = getString(R.string.res_temp_txt_F, tempfl);
                DispTemp.setText(DispText);

                if (CurrentlyMeters == 1) DispText = getString(R.string.res_alt_txt_m, altfl);
                else DispText = getString(R.string.res_alt_txt_f, altfl);
                DispAlt.setText(DispText);

                DispText = getString(R.string.res_pres_txt, pressfl);
                DispPres.setText(DispText);

                DispText = getString(R.string.res_hum_txt, humfl);
                DispHum.setText(DispText);
            }
        });
    }

    // obtain float from Totalmessage received based on the offset
    public float bytetofloat(int offset){
        byte[] bytes = new byte[]{(byte) TotalMessage[offset], (byte) TotalMessage[offset+1], (byte) TotalMessage[offset+2],(byte)  TotalMessage[offset+3]};
        return  ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getFloat();
    }

    // make a HEX string of the received data for debug
    private String byteToHexString(byte[] payload) {
        if (payload == null) return "<empty>";
        StringBuilder stringBuilder = new StringBuilder(payload.length);
        for (byte byteChar : payload)
            stringBuilder.append(String.format("%02X ", byteChar));
        return stringBuilder.toString();
    }

    // handle the received data package
    //    CF 14 00 97 40 42 0E F4 7D 44 C4 87 AA C1 C3 F5 A8 41 01 01
    //    CF 14   magic num
    //    14      TOTAL length
    //    00 97 40 42 0E F4 7D 44 C4 87 AA C1 C3 F5 A8 41 01 01 data
    public void  HandleNotify(final BluetoothGattCharacteristic characteristic) {
         String ff = byteToHexString(characteristic.getValue());
         System.out.println("Display notify "+ ff + "\n");

        int index = 0;

        // check magic number if new message ( offset is zero)
        if (MessageOffset == 0){

            if (characteristic.getValue()[0] != MAGICNUM) {
                updateStatusView(R.string.InvalidPackage, R.color.red, 2000);
                //System.out.println("Invalid pac "+ characteristic.getValue()[0] + "\n");
                return;
            }

            TotalLength = characteristic.getValue()[1] & 0xff;
            index = 2;      // skip magic and total length
        }

        // get length current message
        int Messagelen = characteristic.getValue().length;

        // copy the data
        for ( ; index < Messagelen ; index++, MessageOffset++) {
            TotalMessage[MessageOffset] = characteristic.getValue()[index];
        }

        if (MessageOffset >= TotalLength) {
            display280();
            MessageOffset = 0;            // reset offset for next message
        }
    }

    private final BluetoothGattCallback btleGattCallback = new BluetoothGattCallback() {

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
            System.out.println("gatt call back");
            // this will get called anytime you perform a read or write characteristic operation
            final String charUuid = characteristic.getUuid().toString();

            if (charUuid.equalsIgnoreCase(BME280_TX)) {
                HandleNotify(characteristic);
            } else {
                MainActivity.this.runOnUiThread(() -> statusview.setText("Device read or wrote to " + charUuid + "\n"));
            }
        }

        @SuppressLint("MissingPermission")
        @Override
        public void onConnectionStateChange(final BluetoothGatt gatt, final int status, final int newState) {
            // this will get called when a device connects or disconnects
            switch (newState) {
                case 0:
                    MainActivity.this.runOnUiThread(() -> {
                        disconnectDevice.setVisibility(View.INVISIBLE);
                        stopScanningButton.setVisibility(View.INVISIBLE);
                        startScanningButton.setVisibility(View.VISIBLE);
                        exitnow.setVisibility(View.VISIBLE);
                        sendnow.setVisibility(View.INVISIBLE);
                        setAltitude.setVisibility(View.INVISIBLE);
                        setTemperature.setVisibility(View.INVISIBLE);
                        SetBME280Fields(false);
                    });
                    Toast.makeText(getApplicationContext(), getString(R.string.discon), Toast.LENGTH_SHORT).show();
                    break;
                case 2:
                    MainActivity.this.runOnUiThread(() -> {
                        updateStatusView(R.string.DeviceConnected, R.color.green, 0);
                        disconnectDevice.setVisibility(View.VISIBLE);
                        stopScanningButton.setVisibility(View.INVISIBLE);
                        startScanningButton.setVisibility(View.INVISIBLE);
                        exitnow.setVisibility(View.INVISIBLE);
                    });

                    // discover services and characteristics for this device
                    bluetoothGatt.discoverServices();

                    break;
                default:
                    updateStatusView(R.string.UnknownState, R.color.red, 2000);
                    break;
            }
        }

        @Override
        public void onServicesDiscovered(final BluetoothGatt gatt, final int status) {
            // this will get called after the client initiates a bluetoothGatt.discoverServices() call
            updateStatusView(R.string.DeviceConnect, R.color.green, 0);
            Discoverdevice(bluetoothGatt.getServices());
        }

        @Override
        // Result of a characteristic read operation
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(characteristic);
            }
        }
    };

    private void Discoverdevice(List<BluetoothGattService> gattServices) {

        //System.out.println("---------------Trying to service");
        if (gattServices == null) return;

        // Loops through available GATT Services to detect the right service
        for (BluetoothGattService gattService : gattServices) {
            final String ServiceUuid = gattService.getUuid().toString();
            // look for the right service
            if (BME280_SERVICE.equalsIgnoreCase(ServiceUuid)) {
                updateStatusView(R.string.ServiceDisc, R.color.green, 0);
            }
            else{
                continue;
            }

            new ArrayList<HashMap<String, String>>();
            List<BluetoothGattCharacteristic> gattCharacteristics =
                    gattService.getCharacteristics();

            // Loops through available Characteristics.
            for (BluetoothGattCharacteristic gattCharacteristic :
                    gattCharacteristics) {

                final String charUuid = gattCharacteristic.getUuid().toString();
                // check for BME280 characteristic
                if (charUuid.equalsIgnoreCase(BME280_RX)) {
                    //Turn buttons on
                    MainActivity.this.runOnUiThread(() -> {
                        // display buttons
                        setAltitude.setVisibility(View.VISIBLE);
                        setTemperature.setVisibility(View.VISIBLE);
                        sendnow.setVisibility(View.VISIBLE);
                        // remove scan buttons
                        startScanningButton.setVisibility(View.INVISIBLE);
                        stopScanningButton.setVisibility(View.INVISIBLE);
                        exitnow.setVisibility(View.INVISIBLE);
                    });
                }
                else {
                    if (charUuid.equalsIgnoreCase(BME280_TX)) {
                        //Turn buttons on
                        MainActivity.this.runOnUiThread(new Runnable() {
                            public void run() {
                                // remove scan buttons
                                startScanningButton.setVisibility(View.INVISIBLE);
                                stopScanningButton.setVisibility(View.INVISIBLE);
                                exitnow.setVisibility(View.INVISIBLE);

                                NotificationControl(true);
                            }
                        });

                    } // check notification
                } // else
            }// get next characteristic
        } // get next service
    } // Discoverdevice

    private void broadcastUpdate(final BluetoothGattCharacteristic characteristic) {
        // printout what has been received
        System.out.println("broadcast on" + characteristic.getUuid());
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    // exit programmatically
    public void exitAppCLICK (View view) {
        finishAffinity();
        System.exit(0);
    }
}

