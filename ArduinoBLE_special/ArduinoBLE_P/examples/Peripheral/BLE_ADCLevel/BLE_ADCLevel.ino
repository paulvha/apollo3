/**
 * Parts of this sketch are based on the led peripheral example
 * 
 * You can obtain the ADC information in 2 different ways.
 * 
 * use nRFConnect on your mobile
 * 
 * Streaming (uncomment STREAMING below)
 * scan and look for JMH monitor
 * You need to connect.
 * click on Unknown Service
 * Enable notifications on 19B10002xxxx (press the 3 downarrows)
 * Once you enable (sending 0x1) on 19B10001xxxx  (press the up arrow and enter 01 + send)
 * Every STREAMING second an update of the next ADC line is send
 * Sending 0x00 on 19B10001xxxx will disable sending (press the up arrow and enter 00 + send)
 * 
 * Selective mode
 * scan and look for JMH monitor
 * You need to connect.
 * click on Unknown Service
 * Enable notifications on 19B10002xxxx (press the 3 downarrows)
 * You can now send on 19B10001xxxx   (press the up arrow and enter selection + send)
 *  0x01 (to get ADC1), 
 *  0x02 (to get ADC2), 
 *  0x03 (to get ADC3)
 * 
 * 
 * Version 1.0 / paulvha / November 2022
 */

#include <ArduinoBLE.h>

BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

// characteristic to send the ADC information
BLECharacteristic W_Characteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite |BLENotify, 10);

BLEDevice central;

int valADC1 = 0;
int valADC2 = 0;
int valADC3 = 0;

// define the pins
#define ADC1 A0
#define ADC2 A1
#define ADC3 A2

// Uncomment if you want to use streaming mode. the value defines the seconds in between sending a next update
// 
//#define STREAMING 2

void setup()
{
  Serial.begin(115200);    // initialize serial communication
  while (!Serial);

  // initialize BLE (called in Setup once)
  SetupBLE();
  
  Serial.println("PVH monitor");
}

void loop()
{
    // obtain the values
    valADC1 = analogRead(ADC1);
    valADC2 = analogRead(ADC2);
    valADC3 = analogRead(ADC3);

    // check whether a request has been done
    CheckForBleCmd();
}

/**
 * init BLE (called in Setup once)
 */
void SetupBLE() {
  
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!. Freeze");
    while (1);
  }

   // set advertised local name and service UUID:
  BLE.setLocalName("PVH monitor");
  BLE.setAdvertisedService(ledService);

  // add the characteristic to the service
  ledService.addCharacteristic(switchCharacteristic);    // to receive commands
  ledService.addCharacteristic(W_Characteristic);        // characteristic to send data
  // add service
  BLE.addService(ledService);

  // set the initial value for the characeristic:
  switchCharacteristic.writeValue(0);

  // start advertising
  BLE.advertise();
}


#ifdef STREAMING
/**
 * check for active or deactive streaming mode
 * 
 * The ADC values are send with interval of STREAMING minutes
 */
void CheckForBleCmd()
{
  int l;
  static int cnt = 1;                 // which ADC to send
  static unsigned long st;            // needed for delay between sending
  static bool StreamingMode = false;  // Streaming or not
  uint8_t Sbuf[10];                   // send buffer

  // if NO central connection 
  if (! central) {
    // listen for Bluetooth® Low Energy peripherals to connect:
    central = BLE.central();
    
    if (central) {
      Serial.print("Connected to central: ");
      // print the central's MAC address:
      Serial.println(central.address());
      StreamingMode = false;
    }
    else
      return;
  }

  // if central is still connected to peripheral:
  if (central.connected()) {
    
    // if the remote device wrote to the characteristic,
    // use the value to control the streaming:
    if (switchCharacteristic.written()) {

      if (switchCharacteristic.value()) { // if value other than zero
        Serial.println("Streaming active");
        StreamingMode = true;
        st = millis();
      }
      else {
        Serial.println("Streaming de-actived");
        StreamingMode = false;
      }
    }
  
    if (StreamingMode) {
      
      // wait between sending update
      if (millis() - st < (STREAMING*1000)) return;

      // reset for next time
      st = millis();

      switch(cnt) {
  
        case 1:
          l = sprintf((char *) Sbuf,"ADC1 %d",valADC1);
          cnt++;
          break;
        case 2:       
          l = sprintf((char *) Sbuf,"ADC2 %d",valADC2);
          cnt++;
          break;
        case 3:
          l = sprintf((char *) Sbuf,"ADC3 %d",valADC3);
          cnt = 1;
          break;
      }
            
      Serial.println((char *) Sbuf);
      W_Characteristic.writeValue(Sbuf,l);   
    }

    return;
  } 
 
  // when the central disconnects, print it out:
  Serial.print(F("Disconnected from central: "));
  Serial.println(central.address());
  StreamingMode = false;
  central = BLE.central();
}

#else

/**
 * check something was requested 
 * 
 * The ADC value is only send on selective request
 */
void CheckForBleCmd()
{
  int l;
  uint8_t Sbuf[10];

  // if NO central
  if (! central) {
    // listen for Bluetooth® Low Energy peripherals to connect:
    central = BLE.central();
    
    if (central) {
      Serial.print("Connected to central: ");
      // print the central's MAC address:
      Serial.println(central.address());
    }
    else
      return;
  }

  // if central is still connected to peripheral:
  if (central.connected()) {
    
    // if the remote device wrote to the characteristic,
    // use the value to control the LED:
    if (switchCharacteristic.written()) {
      
      uint8_t Rval = switchCharacteristic.value();

      switch(Rval) {

        case 1:
          l = sprintf((char *) Sbuf,"ADC1 %d",valADC1);
          break;
        case 2:       
          l = sprintf((char *) Sbuf,"ADC2 %d",valADC2);
          break;
        case 3:
          l = sprintf((char *) Sbuf,"ADC3 %d",valADC3);
          break;
        default:
          return;
          break;
      }
      
      Serial.println((char *) Sbuf);
      W_Characteristic.writeValue(Sbuf,l);   
    }

    return;
  }
 
  // when the central disconnects, print it out:
  Serial.print(F("Disconnected from central: "));
  Serial.println(central.address());
  central = BLE.central();
}
#endif // STREAMING
