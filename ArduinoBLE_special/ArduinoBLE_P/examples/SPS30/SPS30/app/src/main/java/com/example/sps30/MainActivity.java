package com.example.sps30;

import static android.icu.util.Calendar.*;

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
import android.icu.util.Calendar;
import android.icu.text.SimpleDateFormat;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.preference.PreferenceActivity;
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

    // GUI definitions
    Button stopScanningButton;
    Button startScanningButton;
    Button button_sendNow;
    Button button_disconnect;
    Button button_wakeup;
    Button button_sleep;
    Button button_clean;
    Button button_exit;

    TextView mass1;
    TextView mass25;
    TextView mass4;
    TextView mass10;
    TextView num1;
    TextView num25;
    TextView num4;
    TextView num10;
    TextView statusview;
    TextView updateTime;
    TextView massheader;
    TextView numheader;

    // bluetooth variables
    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    BluetoothGatt bluetoothGatt;

    final String SPS30_SERVICE = "19B10030-E8F2-537E-4F6C-D104768A1214";
    final String SPS30_RX = "19B10031-E8F2-537E-4F6C-D104768A1214";        // TX for central
    final String SPS30_TX = "19B10032-E8F2-537E-4F6C-D104768A1214";        // RX for central (notify)
    final String CONFIG_DESCRIPTOR = "00002902-0000-1000-8000-00805f9b34fb";
    final String DEVICENAME = "Peripheral SPS30 BLE";

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
    //private static final int REQUEST_START  = '1';
    //private static final int REQUEST_STOP   = '2';
    private static final int REQUEST_CLEAN  = '3';
    private static final int REQUEST_SLEEP  = '4';
    private static final int REQUEST_WAKEUP = '5';
    private static final int REQUEST_NOW    = '6';
    int sps30Status = 0;
    boolean SetStartScanningButton = true;       // if StopScanning is called it will make the scan button VISIBLE

    // handle sps30 message
    int TotalLength = 0;                         // total length (maybe split on multiple messages)
    int[] TotalMessage = new int[100];           // holds the total message (44 are expected)
    int MessageOffset = 0;                       // current offset in TotalMessage
    private static final int MAGICNUM = -49;     // indicate start of NEW message (0xcf)


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // portrait only
        setRequestedOrientation (ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        setContentView(R.layout.activity_main);

        // needed to display results
        mass1 = findViewById(R.id.Mass_1);
        mass25 = findViewById(R.id.Mass_2);
        mass4 = findViewById(R.id.Mass_4);
        mass10 = findViewById(R.id.Mass_10);
        num1 = findViewById(R.id.Num_1);
        num25 = findViewById(R.id.Num_2);
        num4 = findViewById(R.id.Num_4);
        num10 = findViewById(R.id.Num_10);
        statusview = findViewById(R.id.Header);
        updateTime = findViewById(R.id.UpdateTime);
        massheader = findViewById(R.id.Mass);
        numheader = findViewById(R.id.Number);

        startScanningButton = findViewById(R.id.Button_start_scan);
        startScanningButton.setOnClickListener(v -> startScanning());
        startScanningButton.setVisibility(View.VISIBLE);

        stopScanningButton = findViewById(R.id.Button_stop_scan);
        stopScanningButton.setOnClickListener(v -> stopScanning());
        stopScanningButton.setVisibility(View.INVISIBLE);

        button_disconnect = findViewById(R.id.Button_disconnect);
        button_disconnect.setOnClickListener(v -> disconnect());
        button_disconnect.setVisibility(View.INVISIBLE);

        button_sendNow = findViewById(R.id.Button_now);
        button_sendNow.setOnClickListener(v -> sendNow());
        button_sendNow.setVisibility(View.INVISIBLE);

        button_sleep = findViewById(R.id.Button_sleep);
        button_sleep.setOnClickListener(v -> sleepNow());
        button_sleep.setVisibility(View.INVISIBLE);

        button_wakeup = findViewById(R.id.Button_wakeup);
        button_wakeup.setOnClickListener(v -> wakeupNow());
        button_wakeup.setVisibility(View.INVISIBLE);

        button_clean = findViewById(R.id.Button_clean);
        button_clean.setOnClickListener(v -> cleanNow());
        button_clean.setVisibility(View.INVISIBLE);

        button_exit = findViewById(R.id.Button_Exit);
        button_exit.setOnClickListener(v -> exitAppCLICK());
        button_exit.setVisibility(View.VISIBLE);

        SetSPS30Fields(false);

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

    // exit programmatically
    public void exitAppCLICK () {
        finishAffinity();
        System.exit(0);
    }

    // start to scan for device
    public void startScanning() {
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

        Toast.makeText(getApplicationContext(), getString(R.string.scanstart), Toast.LENGTH_SHORT).show();
    }

    // stop scanning
    public void stopScanning() {

        if (SetStartScanningButton) {
            startScanningButton.setVisibility(View.VISIBLE);
            button_exit.setVisibility(View.VISIBLE);
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
                statusview.setText(result.getDevice().getName());
                if (result.getDevice().getName().equalsIgnoreCase(DEVICENAME)) {
                    updateStatusView(R.string.DeviceDetected, R.color.green, 0);
                    DeviceDiscovered = result.getDevice();
                    //System.out.println("Found device");
                    SetStartScanningButton = false;
                    stopScanning();
                    connectToDevice();
                }
            }
        }
    };

    private final BluetoothGattCallback btleGattCallback = new BluetoothGattCallback() {

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
            System.out.println("gatt call back");
            // this will get called anytime you perform a read or write characteristic operation
            final String charUuid = characteristic.getUuid().toString();

            if (charUuid.equalsIgnoreCase(SPS30_TX)) {
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
                        button_disconnect.setVisibility(View.INVISIBLE);
                        stopScanningButton.setVisibility(View.INVISIBLE);
                        startScanningButton.setVisibility(View.VISIBLE);
                        button_exit.setVisibility(View.VISIBLE);
                        button_sendNow.setVisibility(View.INVISIBLE);
                        button_clean.setVisibility(View.INVISIBLE);
                        button_sleep.setVisibility(View.INVISIBLE);
                        button_wakeup.setVisibility(View.INVISIBLE);
                        updateStatusView(R.string.discon, R.color.red, 10);
                        SetSPS30Fields(false);
                    });
                    //Toast.makeText(getApplicationContext(), getString(R.string.discon), Toast.LENGTH_SHORT).show();
                    break;
                case 2:
                    MainActivity.this.runOnUiThread(() -> {
                        updateStatusView(R.string.DeviceConnected, R.color.green, 0);
                        button_disconnect.setVisibility(View.VISIBLE);
                        stopScanningButton.setVisibility(View.INVISIBLE);
                        startScanningButton.setVisibility(View.INVISIBLE);
                        button_exit.setVisibility(View.INVISIBLE);
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
            updateStatusView(R.string.DeviceConnected, R.color.green, 0);
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
        int BothCharFound = 0;
        // Loops through available GATT Services to detect the right service
        for (BluetoothGattService gattService : gattServices) {
            final String ServiceUuid = gattService.getUuid().toString();
            // look for the right service
            if (SPS30_SERVICE.equalsIgnoreCase(ServiceUuid)) {
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
                // check for SPS30 characteristic
                if (charUuid.equalsIgnoreCase(SPS30_RX)) {
                    //Turn buttons on
                    MainActivity.this.runOnUiThread(() -> {
                        // display buttons
                        button_clean.setVisibility(View.VISIBLE);
                        button_sleep.setVisibility(View.VISIBLE);
                        button_wakeup.setVisibility(View.VISIBLE);
                        button_sendNow.setVisibility(View.VISIBLE);
                        // remove scan buttons
                        startScanningButton.setVisibility(View.INVISIBLE);
                        stopScanningButton.setVisibility(View.INVISIBLE);
                        button_exit.setVisibility(View.INVISIBLE);
                    });
                }
                else {
                    if (charUuid.equalsIgnoreCase(SPS30_TX)) {
                        //Turn buttons on
                        MainActivity.this.runOnUiThread(new Runnable() {
                            public void run() {
                                // remove scan buttons
                                startScanningButton.setVisibility(View.INVISIBLE);
                                stopScanningButton.setVisibility(View.INVISIBLE);
                                button_exit.setVisibility(View.INVISIBLE);

                                NotificationControl(true);
                            }
                        });

                    } // check notification characteristic
                } // else
            }// get next characteristic
        } // get next service
    } // Discoverdevice

    private void broadcastUpdate(final BluetoothGattCharacteristic characteristic) {
        // printout what has been received
        System.out.println("broadcast on" + characteristic.getUuid());
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
            Character = bluetoothGatt.getService(UUID.fromString(SPS30_SERVICE)).getCharacteristic(UUID.fromString(SPS30_TX));
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

    // disconnect and reset
    @SuppressLint("MissingPermission")
    public void disconnect() {
        // called for disconnect button
        NotificationControl(false);
        button_wakeup.setVisibility(View.INVISIBLE);
        button_sleep.setVisibility(View.INVISIBLE);
        button_sendNow.setVisibility(View.INVISIBLE);
        button_clean.setVisibility(View.INVISIBLE);
        SetStartScanningButton = true;
        startScanningButton.setVisibility(View.VISIBLE);
        button_exit.setVisibility(View.VISIBLE);
        button_disconnect.setVisibility(View.INVISIBLE);
        SetSPS30Fields(false);
        bluetoothGatt.disconnect();

        Toast.makeText(getApplicationContext(),getString(R.string.discon),Toast.LENGTH_SHORT).show();
    }

    // write command to the characteristic
    @SuppressLint("MissingPermission")
    public boolean writeCommand(int Command) {

        BluetoothGattCharacteristic Character = null;

        if (bluetoothGatt != null) {
            Character = bluetoothGatt.getService(UUID.fromString(SPS30_SERVICE)).getCharacteristic(UUID.fromString(SPS30_RX));
        }

        if (Character == null) {
            updateStatusView(R.string.SendCharNotFound, R.color.red, 5000);
            return false;
        }

        Character.setValue(Command, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        return (bluetoothGatt.writeCharacteristic(Character));
    }

    ///////////////////////// PROGRAMME FUNCTIONS /////////////////////////////////////////////////

    public void sendNow() {
        writeCommand(REQUEST_NOW);
    }

    public void sleepNow() {writeCommand(REQUEST_SLEEP); }

    public void wakeupNow() {
        writeCommand(REQUEST_WAKEUP);
    }

    public void cleanNow() {
        writeCommand(REQUEST_CLEAN);
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
    //    CF  magic num
    //    14  TOTAL length
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
            displaySPS30();
            MessageOffset = 0;            // reset offset for next message
        }
    }

    // display the received SPS30 information
    /* format data in the message is

    OFFSET
    0   float   MassPM1;        // Mass Concentration PM1.0 [μg/m3]
    4   float   MassPM2;        // Mass Concentration PM2.5 [μg/m3]
    8   float   MassPM4;        // Mass Concentration PM4.0 [μg/m3]
    12  float   MassPM10;       // Mass Concentration PM10 [μg/m3]
    16  float   NumPM0;         // Number Concentration PM0.5 [#/cm3]
    20  float   NumPM1;         // Number Concentration PM1.0 [#/cm3]
    24  float   NumPM2;         // Number Concentration PM2.5 [#/cm3]
    28  float   NumPM4;         // Number Concentration PM4.0 [#/cm3]
    32  float   NumPM10;        // Number Concentration PM4.0 [#/cm3]
    36  float   PartSize;       // Typical Particle Size [μm]
    40  byte    Status;         // sleep (0), wakeup (1);

    total 41 bytes
    */

    public void displaySPS30() {
        // display the results received (called from notification)
        final float MassPM1  = bytetofloat(0);
        final float MassPM2  = bytetofloat(4);
        final float MassPM4  = bytetofloat(8);
        final float MassPM10 = bytetofloat(12);
        final float NumPM0   = bytetofloat(16);
        final float NumPM1   = bytetofloat(20);
        final float NumPM2   = bytetofloat(24);
        final float NumPM4   = bytetofloat(28);
        final float NumPM10  = bytetofloat(32);
        final float Partsize = bytetofloat(36);

        // Sleep status
        sps30Status = TotalMessage[40];

        if (sps30Status == 0){
            updateStatusView(R.string.SleepStatus, R.color.holo_blue_light, 0);
            return;
        }

        SetSPS30Fields(true);

        MainActivity.this.runOnUiThread(new Runnable() {
            public void run() {
                statusview.setText(R.string.empty);
                String DispText;
                updateStatusView(R.string.NewUpdate, R.color.holo_blue_light, 2000);
                Calendar c = getInstance();
                SimpleDateFormat df = new SimpleDateFormat("HH:mm:ss");
                String formattedDate = df.format(c.getTime());
                DispText = getString(R.string.UpdateTime, formattedDate);
                updateTime.setText(DispText);

                DispText = getString(R.string.MassFormat, MassPM1);
                mass1.setText(DispText);

                DispText = getString(R.string.MassFormat, MassPM2);
                mass25.setText(DispText);

                DispText = getString(R.string.MassFormat, MassPM4);
                mass4.setText(DispText);

                DispText = getString(R.string.MassFormat, MassPM10);
                mass10.setText(DispText);

                DispText = getString(R.string.NumFormat, NumPM1);
                num1.setText(DispText);

                DispText = getString(R.string.NumFormat, NumPM2);
                num25.setText(DispText);

                DispText = getString(R.string.NumFormat, NumPM4);
                num4.setText(DispText);

                DispText = getString(R.string.NumFormat, NumPM10);
                num10.setText(DispText);
            }
        });
    }

    // show or hide sps30 result fields
    public void SetSPS30Fields(boolean cmd){
        MainActivity.this.runOnUiThread(() -> {
            if(cmd) {
                mass1.setVisibility((View.VISIBLE));
                mass25.setVisibility((View.VISIBLE));
                mass4.setVisibility((View.VISIBLE));
                mass10.setVisibility((View.VISIBLE));
                num1.setVisibility((View.VISIBLE));
                num25.setVisibility((View.VISIBLE));
                num4.setVisibility((View.VISIBLE));
                num10.setVisibility((View.VISIBLE));
                updateTime.setVisibility((View.VISIBLE));
                massheader.setVisibility((View.VISIBLE));
                numheader.setVisibility((View.VISIBLE));
            }
            else {
                mass1.setVisibility((View.INVISIBLE));
                mass25.setVisibility((View.INVISIBLE));
                mass4.setVisibility((View.INVISIBLE));
                mass10.setVisibility((View.INVISIBLE));
                num1.setVisibility((View.INVISIBLE));
                num25.setVisibility((View.INVISIBLE));
                num4.setVisibility((View.INVISIBLE));
                num10.setVisibility((View.INVISIBLE));
                updateTime.setVisibility((View.INVISIBLE));
                statusview.setText(R.string.empty);
                massheader.setVisibility((View.INVISIBLE));
                numheader.setVisibility((View.INVISIBLE));
            }
        });
    }

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


}