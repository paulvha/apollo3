/*
  LED / Output / input

  Version 1.0 / December 2022 / paulvha
  * initial version

  This example creates a Bluetooth® Low Energy peripheral with service that contains 
  A characteristic to reveive commands to control :
    The onboard LED
    Set output high / low on 2 different pins
    Get input from one digital & one analog pin.
    Request simulated battery level

  On the notify characteristic it will send :
    Status input from one digital & one analog pin.
    Simulated battery level every x minutes
    
  To those output pins you can connect a relay or other LEDs.

  It is an extension to the default / standard LED peripheral. 

  This sketch accepts the following commands:
  int     action
 =====    ===========
  0x10    OutPin1 LOW
  0x11    OutPin1 HIGH
  0x20    OutPin2 LOW
  0x21    OutPin2 HIGH
  0x30    LED OFF
  0x31    LED ON
  0x40    Stop simulated battery
  0x41    Start simulated battery
  0x51    read digital pin1
  0x61    read Analog pin1

  As a bonus it can be controlled from an Android device. The App is made available as part of the code and can be compiled in Android Studio
  
  You can use a generic Bluetooth® Low Energy central app, like LightBlue (iOS and Android) or
  nRF Connect (Android), to interact with the services and characteristics
  created in this sketch.

  This example code is in the public domain.
*/

#include <ArduinoBLE_P.h>

BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

// Not using the "official" battery-UID,  remote clients will be able to get notifications if this characteristic changes
BLECharacteristic LevelChar("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 3);

////////////////////////////////////////////////////////////////////////////////
// This a definition of pins for Sparkfun Artemis ATP. Adjust to your board  //
///////////////////////////////////////////////////////////////////////////////
const int ledPin = LED_BUILTIN; // pin to use for the LED
const int OutPin1 = D28;        // define output pins where you can connect a led or relay
const int OutPin2 = D27;

const int InPin1 = D23;         // define digital input pin to read
const int AnPin1 = A29;         // define analog input pin to read

// time between sending simulated battery (mS)
#define WaitBetweenSending  5000

///////////////////////////////////////////////////
// No change needed beyond this point
///////////////////////////////////////////////////

// define commands from remote / central
const int CmdLedOn   = 0x31;
const int CmdLedOff  = 0x30;
const int CmdOut1On  = 0x11;
const int CmdOut1Off = 0x10;
const int CmdOut2On  = 0x21;
const int CmdOut2Off = 0x20;
const int CmdBatOn   = 0x41;
const int CmdBatOff  = 0x40;  
const int CmdIn1Rd   = 0x51;  // read input digital pin1
const int CmdAn1Rd   = 0x61;  // read input analog pin1

// enable / disable sending simulated battery 
bool DoBatNotification = false;
unsigned long StartBat = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  // BLE.debug(Serial); // uncomment to enable debug messages 

  Serial.println("ExampleXX1 Output / Input control");

  // set LED pin to output mode
  pinMode(ledPin, OUTPUT);

  // set external output
  pinMode(OutPin1, OUTPUT);
  pinMode(OutPin2, OUTPUT);

  // set input
  pinMode(InPin1, INPUT);
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed! freeze");
    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("Output_Input_Control");
  BLE.setAdvertisedService(ledService);

  // add the characteristic to the service
  ledService.addCharacteristic(switchCharacteristic);
  ledService.addCharacteristic(LevelChar);     // add the level characteristic

  // add service
  BLE.addService(ledService);

  // set the initial value for the characeristic:
  switchCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();

  Serial.println("BLE Output / Input control Peripheral is ready");
}

void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
      // if the remote device wrote to the characteristic,

      if (switchCharacteristic.written()) {
        int val = switchCharacteristic.value();

       // Serial.printf("something written %x %d\n", val, val);
        
        switch (val) {
          case 0x00 :
          case CmdLedOff :
            Serial.println("LED off");
            digitalWrite(ledPin, LOW);         // will turn the LED off
            break;
            
          case CmdLedOn :
            Serial.println("LED on");
            digitalWrite(ledPin, HIGH);         // will turn the LED on
            break;
            
          case CmdOut1Off :
            Serial.println("OutPin1 off");
            digitalWrite(OutPin1, LOW);         // will turn OutPin1 off
            break;
            
          case CmdOut1On :
            Serial.println("OutPin1 on");
            digitalWrite(OutPin1, HIGH);         // will turn OutPin1 on
            break;
            
          case CmdOut2Off :
            Serial.println("OutPin2 off");
            digitalWrite(OutPin2, LOW);         // will turn OutPin2 off
            break;
            
          case CmdOut2On :
            Serial.println("OutPin2 on");
            digitalWrite(OutPin2, HIGH);         // will turn OutPin2 on
            break;

          case CmdBatOn :
            Serial.println("Start simulated battery notification");
            DoBatNotification = true;
            StartBat = 0;
            break;

          case CmdBatOff :
            Serial.println("Stop simulated battery notification");
            DoBatNotification = false;
            break;

          case CmdIn1Rd :
            Serial.println("Read Digital input 1");
            UpdateLevel(CmdIn1Rd);
            break;

          case CmdAn1Rd :
            Serial.println("Read Analog input 1");
            UpdateLevel(CmdAn1Rd);
            break;
            
          default :
            Serial.print("Unknown command 0x");
            Serial.println(val,HEX);
            break;
        }
      }

      if (DoBatNotification) {
        UpdateLevel(CmdBatOn);
      }
    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
    DoBatNotification = false;               // disable notifications
  }
}

/**
 * Update level to central for digital, analog and simulated batttery
 * @param i : the command to execute
 */
void UpdateLevel(int i) {
  uint8_t data[3];          // max size as defined with characteristic

  data[0] = i;              // indicate which command response

  if (i == CmdBatOn) {      // simulated battery level between 20 and 90
    static int batteryLevel = 20;
    
    if (millis() - StartBat < WaitBetweenSending) return;
    
    StartBat = millis();
    data[1] = batteryLevel;
      
    LevelChar.writeValue(data, 2);  // update the battery level characteristic
  
    Serial.print("Battery Level % is now: "); // print it
    Serial.println(batteryLevel);
  
    if (batteryLevel++ > 90) batteryLevel = 20 ;
  }
  
  else if (i == CmdAn1Rd) { // read analog pin
    int Anpin = analogRead(AnPin1);
    
    data[1] = Anpin >> 8 & 0xff;      // MSB
    data[2] = Anpin & 0xff;           // LSB
      
    LevelChar.writeValue(data, 3);    // update level characteristic
  
    Serial.print("Analog pin 1 Level is "); // print it
    Serial.println(Anpin);
  }
  else if (i == CmdIn1Rd) {
    data[1] = digitalRead(InPin1);
  
    LevelChar.writeValue(data, 2);  // update level characteristic
    
    Serial.print("Digital pin 1 Level is "); // print it
    Serial.println(data[1]);
  }
  /*
  else if (i == xx) {
    // in case you want to use a real input
    int battery = analogRead(A0);
    int batteryLevel = map(battery, 0, 1023, 0, 100);
  
    if (batteryLevel != oldBatteryLevel) {         // if the battery level has changed

      data[1] = batteryLevel;
      LevelChar.writeValue(data, 2);  // update the battery level characteristic

      Serial.print("Battery Level % is now: ");    // print it
      Serial.println(batteryLevel);
      
      oldBatteryLevel = batteryLevel;              // save the level for next comparison
    }
  }
  */
}
