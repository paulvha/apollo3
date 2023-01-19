package com.example.ble_led_relay;
/*
 This app is designed to work with the ArduinoBLE sketch LedInputOutput.
 It will demonstrate the following :
   scan for available devices / peripherals
   connect to peripheral
   if the right service, characteristics are discovered it will display button
   you can request :
     Regular simulated battery update
     Status on an Analog input pin
     Status on a digital input pin
     Switch the peripheral onboard led on/off
     Switch 2 different output pins HIGH / LOW
   Disconnect in the right way

   Version 1.0 / December 2022 / Paulvha
 */
import android.Manifest;
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
import android.content.pm.PackageManager;
import android.os.AsyncTask;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.view.inputmethod.InputMethodManager;
import com.google.android.gms.appindexing.AppIndex;
import com.google.android.gms.common.api.GoogleApiClient;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    //****** GUI Components ******
    // switch buttons
    Button startScanningButton;
    Button stopScanningButton;
    Button connectToDevice;
    Button disconnectDevice;
    Button setLedOn;
    Button setLedOff;
    Button setOut1On;
    Button setOut1off;
    Button setOut2On;
    Button setOut2off;
    Button setBatOn;
    Button setBatOff;
    Button getInp1;
    Button getAnp1;

    // text output
    TextView peripheralTextView;
    TextView topLine;

    // input keyboard
    EditText deviceIndexInput;

    // like "defines" in C++
    private static final int CmdLedOn = 0x31;
    private static final int CmdLedOff = 0x30;
    private static final int CmdOut1On = 0x11;
    private static final int CmdOut1Off = 0x10;
    private static final int CmdOut2On = 0x21;
    private static final int CmdOut2Off = 0x20;
    private static final int CmdBatOn = 0x41;
    private static final int CmdBatOff = 0x40;
    private static final int CmdIn1Rd   = 0x51;
    private static final int CmdAn1Rd   = 0x61;

    // bluetooth variables
    BluetoothManager btManager;
    BluetoothAdapter btAdapter;
    BluetoothLeScanner btScanner;
    BluetoothGatt bluetoothGatt;

    final String SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
    final String ledCharUuid = "19B10001-E8F2-537E-4F6C-D104768A1214";  // led and output control
    final String ADCCharUuid = "19B10002-E8F2-537E-4F6C-D104768A1214";  // notify receive battery
    final String CONFIG_DESCRIPTOR = "00002902-0000-1000-8000-00805f9b34fb";

    private final static int REQUEST_ENABLE_BT = 1;
    private static final int PERMISSION_REQUEST_COARSE_LOCATION = 1;

    Boolean btScanning = false;
    int deviceIndex = 0;
    ArrayList<BluetoothDevice> devicesDiscovered = new ArrayList<>();

    public final static String ACTION_DATA_AVAILABLE =
            "com.example.bluetooth.le.ACTION_DATA_AVAILABLE";

    // Stops scanning after 5 seconds.
    private final Handler mHandler = new Handler();
    private static final long SCAN_PERIOD = 5000;
    private View relativeLayout;            // to change background color

    /**
     * ATTENT ON: This was auto-generated to implement the App Indexing API.
     * See https://g.co/AppIndexing/AndroidStudio for more information.
     */
    private GoogleApiClient client;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        topLine = (TextView) findViewById(R.id.TopLine);
        relativeLayout = findViewById(R.id.rlVar1);

        peripheralTextView = (TextView) findViewById(R.id.PeripheralTextView);
        peripheralTextView.setMovementMethod(new ScrollingMovementMethod());

        deviceIndexInput = (EditText) findViewById(R.id.InputIndex);
        deviceIndexInput.setText("0");

        connectToDevice = (Button) findViewById(R.id.ConnectButton);
        connectToDevice.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                connectToDeviceSelected();
            }
        });

        disconnectDevice = (Button) findViewById(R.id.DisconnectButton);
        disconnectDevice.setVisibility(View.INVISIBLE);
        disconnectDevice.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                disconnectDeviceSelected();
            }
        });

        startScanningButton = (Button) findViewById(R.id.StartScanButton);
        startScanningButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { startScanning(); }
        });

        stopScanningButton = (Button) findViewById(R.id.StopScanButton);
        stopScanningButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { stopScanning(); }
        });
        stopScanningButton.setVisibility(View.INVISIBLE);

        getInp1 =  (Button) findViewById(R.id.GetInp1);
        getInp1.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { GetInput(CmdIn1Rd); }
        });
        getInp1.setVisibility(View.INVISIBLE);

        getAnp1 =  (Button) findViewById(R.id.GetAnp1);
        getAnp1.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { GetInput(CmdAn1Rd); }
        });
        getAnp1.setVisibility(View.INVISIBLE);

        setLedOn =  (Button) findViewById(R.id.SetLedOn);
        setLedOn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { OutOn(CmdLedOn);     }
        });
        setLedOn.setVisibility(View.INVISIBLE);

        setLedOff =  (Button) findViewById(R.id.SetLedOff);
        setLedOff.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { OutOff(CmdLedOff); }
        });
        setLedOff.setVisibility(View.INVISIBLE);

        setOut1On =  (Button) findViewById(R.id.SetOut1On);
        setOut1On.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { OutOn(CmdOut1On);}
        });
        setOut1On.setVisibility(View.INVISIBLE);

        setOut1off =  (Button) findViewById(R.id.SetOut1Off);
        setOut1off.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { OutOff(CmdOut1Off); }
        });
        setOut1off.setVisibility(View.INVISIBLE);

        setOut2On =  (Button) findViewById(R.id.SetOut2On);
        setOut2On.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { OutOn(CmdOut2On); }
        });
        setOut2On.setVisibility(View.INVISIBLE);

        setOut2off =  (Button) findViewById(R.id.SetOut2Off);
        setOut2off.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { OutOff(CmdOut2Off); }
        });
        setOut2off.setVisibility(View.INVISIBLE);

        setBatOn =  (Button) findViewById(R.id.SetBatOn);
        setBatOn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {OutOn(CmdBatOn); }
        });
        setBatOn.setVisibility(View.INVISIBLE);

        setBatOff =  (Button) findViewById(R.id.SetBatOff);
        setBatOff.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) { OutOff(CmdBatOff); }
        });
        setBatOff.setVisibility(View.INVISIBLE);

        btManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        btAdapter = btManager.getAdapter();
        btScanner = btAdapter.getBluetoothLeScanner();

        if (btAdapter != null && !btAdapter.isEnabled()) {
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        }

        // Make sure we have access coarse location enabled, if not, prompt the user to enable it
        if (this.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            final AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("This app needs location access");
            builder.setMessage("Please grant location access so this app can detect peripherals.");
            builder.setPositiveButton(android.R.string.ok, null);
            builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialog) {
                    requestPermissions(new String[]{Manifest.permission.ACCESS_COARSE_LOCATION}, PERMISSION_REQUEST_COARSE_LOCATION);
                }
            });
            builder.show();
        }

        client = new GoogleApiClient.Builder(this).addApi(AppIndex.API).build();
    }

    // Device scan callback.
    private final ScanCallback leScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            if (result.getDevice().getName() != null) {
                peripheralTextView.append("Index: " + deviceIndex + "," + result.getDevice().getName() + " rssi: " + result.getRssi() + "\n");
                devicesDiscovered.add(result.getDevice());
                deviceIndex++;
                // auto scroll for text view
                final int scrollAmount = peripheralTextView.getLayout().getLineTop(peripheralTextView.getLineCount()) - peripheralTextView.getHeight();
                // if there is no need to scroll, scrollAmount will be <=0
                if (scrollAmount > 0) {
                    peripheralTextView.scrollTo(0, scrollAmount);
                }
            }
        }
    };

    // this will display enabled notifications
    public void  DisplayNotify(final BluetoothGattCharacteristic characteristic) {
        // 2 bytes 0 = source  1 = value
        System.out.println("display notify");
        int source = characteristic.getValue()[0] & 0xff;
        int val = characteristic.getValue()[1] & 0xff;

        if (source == CmdBatOn) {
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("BatteryLevel: " + val + "%\n");
                }
            });
        }
        else if (source == CmdIn1Rd) {
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    if (val == 1)
                        peripheralTextView.append("InputPin1 level HIGH.\n");
                    else
                        peripheralTextView.append("InputPin1 level LOW.\n");
                }
            });
        }
        else if (source == CmdAn1Rd) {
            int val16 = (val << 8 ) | characteristic.getValue()[2] & 0xff;
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Analog Pin1 level is: " + val16+".\n");}
            });
        }
    }

    // Device connect call back
    private final BluetoothGattCallback btleGattCallback = new BluetoothGattCallback() {

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, final BluetoothGattCharacteristic characteristic) {
            System.out.println("gatt call back");
            // this will get called anytime you perform a read or write characteristic operation
            final String charUuid = characteristic.getUuid().toString();

            if (charUuid.equalsIgnoreCase(ADCCharUuid)){
                DisplayNotify(characteristic);
            }
            else {
                MainActivity.this.runOnUiThread(new Runnable() {
                    public void run() {
                        peripheralTextView.append("device read or wrote to " + charUuid + "\n");
                    }
                });
            }
        }

        @Override
        public void onConnectionStateChange(final BluetoothGatt gatt, final int status, final int newState) {
            // this will get called when a device connects or disconnects
            System.out.println(newState);
            switch (newState) {
                case 0:
                    MainActivity.this.runOnUiThread(new Runnable() {
                        public void run() {
                            connectToDevice.setVisibility(View.VISIBLE);
                            disconnectDevice.setVisibility(View.INVISIBLE);
                            setLedOff.setVisibility(View.INVISIBLE);
                            setLedOn.setVisibility(View.INVISIBLE);
                            setOut1off.setVisibility(View.INVISIBLE);
                            setOut1On.setVisibility(View.INVISIBLE);
                            setOut2off.setVisibility(View.INVISIBLE);
                            setOut1off.setVisibility(View.INVISIBLE);
                            getInp1.setVisibility(View.INVISIBLE);
                            getAnp1.setVisibility(View.INVISIBLE);
                            setBatOn.setVisibility(View.INVISIBLE);
                            setBatOff.setVisibility(View.INVISIBLE);
                            startScanningButton.setVisibility(View.VISIBLE);
                        }
                    });
                    Toast.makeText(getApplicationContext(),getString(R.string.discon),Toast.LENGTH_SHORT).show();
                    break;
                case 2:
                    MainActivity.this.runOnUiThread(new Runnable() {
                        public void run() {
                            peripheralTextView.append("device connected\n");
                            connectToDevice.setVisibility(View.INVISIBLE);
                            disconnectDevice.setVisibility(View.VISIBLE);
                        }
                    });

                    // discover services and characteristics for this device
                    bluetoothGatt.discoverServices();

                    break;
                default:
                    MainActivity.this.runOnUiThread(new Runnable() {
                        public void run() {
                            peripheralTextView.append("we got an unknown state, uh oh\n");
                        }
                    });
                    break;
            }
        }

        @Override
        public void onServicesDiscovered(final BluetoothGatt gatt, final int status) {
            // this will get called after the client initiates a bluetoothGatt.discoverServices() call
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("device services have been discovered\n");
                }
            });
            displayGattServices(bluetoothGatt.getServices());
        }

        @Override
        // Result of a characteristic read operation
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            }
        }
    };

    private void broadcastUpdate(final String action,
                                 final BluetoothGattCharacteristic characteristic) {
        // printout what has been received
        System.out.println(characteristic.getUuid());
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
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
                return;
            }
        }
    }

    // write command to the characteristic
    public boolean writeCommand(int Command) {

        // BluetoothGattService Service = null;
        BluetoothGattCharacteristic Character = null;

        if (bluetoothGatt != null) {
            Character = bluetoothGatt.getService(UUID.fromString(SERVICE_UUID)).getCharacteristic(UUID.fromString(ledCharUuid));
        }

        if (Character == null) {
            System.out.println("Characteristic not found!");
            return false;
        }

        Character.setValue(Command, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        return (bluetoothGatt.writeCharacteristic(Character));
    }

    // send request to get digital or analog input pin value
    public void GetInput(int cmd) {
        System.out.println("Get Input :" + cmd);

        if (cmd == CmdIn1Rd) {  // Digital pin 1
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Get digital InputPin 1.\n");
                }
            });
        }
        else if (cmd == CmdAn1Rd) {  // analog pin 1
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Get analog InputPin 1.\n");
                }
            });
        }

        if ( ! writeCommand(cmd) ) {
            //System.out.println("ERROR requesting input channel");
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.setText("Error during request.\n");
                }
            });
        }

        final int scrollAmount = peripheralTextView.getLayout().getLineTop(peripheralTextView.getLineCount()) - peripheralTextView.getHeight();
        // if there is no need to scroll, scrollAmount will be <=0
        if (scrollAmount > 0) {
            peripheralTextView.scrollTo(0, scrollAmount);
        }

    }

    // set request to set an activity 'on'
    public void OutOn(int CommandOn) {
        System.out.println("Set on "+CommandOn);

        if ( CommandOn == CmdOut1On ){  // out 1
            setOut1On.setVisibility(View.INVISIBLE);
            setOut1off.setVisibility(View.VISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Output1 on\n");
                }
            });
        }
        else if ( CommandOn == CmdOut2On ){  // out 2
            setOut2On.setVisibility(View.INVISIBLE);
            setOut2off.setVisibility(View.VISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Output2 on\n");
                }
            });
        }
        else if ( CommandOn == CmdLedOn ){  // led
            setLedOn.setVisibility(View.INVISIBLE);
            setLedOff.setVisibility(View.VISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Led on\n");
                }
            });
        }
        else if ( CommandOn == CmdBatOn ){  // enable battery notification
            setBatOn.setVisibility(View.INVISIBLE);
            setBatOff.setVisibility(View.VISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Battery level on\n");
                }
            });
        }

        if ( ! writeCommand(CommandOn) ) {
            //System.out.println("ERROR set channel on");
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.setText("Error set on");
                }
            });
        }

        final int scrollAmount = peripheralTextView.getLayout().getLineTop(peripheralTextView.getLineCount()) - peripheralTextView.getHeight();
        // if there is no need to scroll, scrollAmount will be <=0
        if (scrollAmount > 0) {
            peripheralTextView.scrollTo(0, scrollAmount);
        }
    }

    // handle a request to set an activity off
    public void OutOff(int CommandOn) {
        System.out.println("Sent off "+CommandOn);

        if ( CommandOn == CmdOut1Off ){
            setOut1On.setVisibility(View.VISIBLE);
            setOut1off.setVisibility(View.INVISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Output1 off\n");
                }
            });
        }
        else if ( CommandOn == CmdOut2Off ){
            setOut2On.setVisibility(View.VISIBLE);
            setOut2off.setVisibility(View.INVISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Output2 off\n");
                }
            });
        }
        else if ( CommandOn == CmdLedOff){      // led
            setLedOn.setVisibility(View.VISIBLE);
            setLedOff.setVisibility(View.INVISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Led off\n");
                }
            });
        }
        else if (CommandOn == CmdBatOff){      // battery
            setBatOn.setVisibility(View.VISIBLE);
            setBatOff.setVisibility(View.INVISIBLE);
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.append("Setting Battery off\n");
                }
            });
        }

        if ( ! writeCommand(CommandOn) ) {
            //System.out.println("ERROR set channel off");
            MainActivity.this.runOnUiThread(new Runnable() {
                public void run() {
                    peripheralTextView.setText("Error setting off");
                }
            });
        }

        final int scrollAmount = peripheralTextView.getLayout().getLineTop(peripheralTextView.getLineCount()) - peripheralTextView.getHeight();
        // if there is no need to scroll, scrollAmount will be <=0
        if (scrollAmount > 0) {
            peripheralTextView.scrollTo(0, scrollAmount);
        }
    }
    // remove the softkey board from the screen
    private  void closeKeyboard()
    {
        //https://www.geeksforgeeks.org/how-to-programmatically-hide-android-soft-keyboard/
        // this will give us the view which is currently focus in this layout
        View view = this.getCurrentFocus();

        // if nothing is currently focus then this will protect the app from crash
        if (view != null) {
            // now assign the system service to InputMethodManager
            InputMethodManager manager = (InputMethodManager)
            getSystemService(Context.INPUT_METHOD_SERVICE);
            manager.hideSoftInputFromWindow( view.getWindowToken(), 0);
        }

        peripheralTextView.setVisibility(View.VISIBLE);
    }

    public void startScanning() {
        System.out.println("start scanning");
        btScanning = true;
        deviceIndex = 0;
        devicesDiscovered.clear();
        peripheralTextView.setText("");
        topLine.setText("Select your device");
        startScanningButton.setVisibility(View.INVISIBLE);
        stopScanningButton.setVisibility(View.VISIBLE);
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                btScanner.startScan(leScanCallback);
            }
        });

        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                stopScanning();
            }
        }, SCAN_PERIOD);

        Toast.makeText(getApplicationContext(),getString(R.string.scanstart),Toast.LENGTH_SHORT).show();
    }

    public void stopScanning() {
        System.out.println("stopping scanning");
        btScanning = false;
        startScanningButton.setVisibility(View.VISIBLE);
        stopScanningButton.setVisibility(View.INVISIBLE);
        AsyncTask.execute(new Runnable() {
            @Override
            public void run() {
                btScanner.stopScan(leScanCallback);
            }
        });

        Toast.makeText(getApplicationContext(),getString(R.string.scanstop),Toast.LENGTH_SHORT).show();
    }

    public void connectToDeviceSelected() {
        peripheralTextView.append("Trying to connect to device at index: " + deviceIndexInput.getText() + "\n");
        int deviceSelected = Integer.parseInt(deviceIndexInput.getText().toString());
        peripheralTextView.append("device " + deviceSelected + devicesDiscovered.get(deviceSelected).getName() + "\n");
        bluetoothGatt = devicesDiscovered.get(deviceSelected).connectGatt(this, false, btleGattCallback);
        deviceIndexInput.setText("0");
        closeKeyboard();
    }

    public void disconnectDeviceSelected() {
        // called for disconnect button
        peripheralTextView.setText("");
        topLine.setText("");
        setLedOff.setVisibility(View.INVISIBLE);
        setLedOn.setVisibility(View.INVISIBLE);
        setOut1On.setVisibility(View.INVISIBLE);
        setOut1off.setVisibility(View.INVISIBLE);
        setOut2On.setVisibility(View.INVISIBLE);
        setOut2off.setVisibility(View.INVISIBLE);
        setBatOn.setVisibility(View.INVISIBLE);
        setBatOff.setVisibility(View.INVISIBLE);
        getInp1.setVisibility(View.INVISIBLE);
        getAnp1.setVisibility(View.INVISIBLE);
        startScanningButton.setVisibility(View.VISIBLE);
        relativeLayout.setBackgroundResource(R.color.green);
        bluetoothGatt.disconnect();

        Toast.makeText(getApplicationContext(),getString(R.string.discon),Toast.LENGTH_SHORT).show();
    }

    private void displayGattServices(List<BluetoothGattService> gattServices) {
        if (gattServices == null) return;

        // Loops through available GATT Services to detect the right service
        for (BluetoothGattService gattService : gattServices) {
            final String ServiceUuid = gattService.getUuid().toString();
            // look for the right service
            if (SERVICE_UUID.equalsIgnoreCase(ServiceUuid)) {
                MainActivity.this.runOnUiThread(new Runnable() {
                    public void run() {
                        peripheralTextView.append("control service has been discovered\n");
                    }
                });
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
                // check for led characteristic
                if (charUuid.equalsIgnoreCase(ledCharUuid)) {
                    //Turn buttons on
                    MainActivity.this.runOnUiThread(new Runnable() {
                        public void run() {
                            peripheralTextView.setText("");
                            topLine.setText("Use Buttons");

                            // display buttons
                            setLedOff.setVisibility(View.INVISIBLE);
                            setLedOn.setVisibility(View.VISIBLE);
                            setOut1off.setVisibility(View.INVISIBLE);
                            setOut1On.setVisibility(View.VISIBLE);
                            setOut2off.setVisibility(View.INVISIBLE);
                            setOut2On.setVisibility(View.VISIBLE);

                            // remove scan buttons
                            startScanningButton.setVisibility(View.INVISIBLE);
                            stopScanningButton.setVisibility(View.INVISIBLE);
                            relativeLayout.setBackgroundResource(R.color.cool);
                        }
                    });
                }
                else {
                    if (charUuid.equalsIgnoreCase(ADCCharUuid)) {
                        //Turn buttons on
                        MainActivity.this.runOnUiThread(new Runnable() {
                            public void run() {
                                peripheralTextView.setText("");

                                // remove scan buttons
                                startScanningButton.setVisibility(View.INVISIBLE);
                                stopScanningButton.setVisibility(View.INVISIBLE);
                                setBatOff.setVisibility(View.INVISIBLE);
                                setBatOn.setVisibility(View.VISIBLE);
                                getInp1.setVisibility(View.VISIBLE);
                                getAnp1.setVisibility(View.VISIBLE);

                                NotificationControl(true);
                            }
                        });

                    } // check ADCCharUUID
                } // else
            }// get next characteristic
        } // get next service
    } // displayGattServices

    public boolean NotificationControl(boolean enable) {
        // enable / disable notifications
        BluetoothGattCharacteristic Character = null;

        if (bluetoothGatt != null) {
            Character = bluetoothGatt.getService(UUID.fromString(SERVICE_UUID)).getCharacteristic(UUID.fromString(ADCCharUuid));
        }

        if (Character == null) {
            System.out.println("Notify characteristic not found!");
            setBatOn.setVisibility(View.INVISIBLE);
            setBatOff.setVisibility(View.INVISIBLE);
            return false;
        }

        bluetoothGatt.setCharacteristicNotification(Character,true);

        BluetoothGattDescriptor desc = Character.getDescriptor(UUID.fromString(CONFIG_DESCRIPTOR));

        if (desc == null) {
            System.out.println("CCD descriptor not found!");
            setBatOn.setVisibility(View.INVISIBLE);
            setBatOff.setVisibility(View.INVISIBLE);
            return false;
        }

        desc.setValue(enable ? BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE : new byte[] { 0x00, 0x00 });
        bluetoothGatt.writeDescriptor(desc);

        return true;
    }

    @Override
    public void onStart() {
        super.onStart();
        client.connect();

    }

    @Override
    public void onStop() {
        super.onStop();
        client.disconnect();
    }
}
