/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 
    # BLE Gatt Client example
     
    This application demonstrates detailed uses of the GattClient APIs. 

    It works in conjunction with the peripheral MBED-BLE_example14_gattservBME280
     
    When the application is started looking for a device called MBED_BME280 and when
    discovered this application reads the value of the characteristics 
    discovered and subscribes to the characteristic 19B10010-E8F2-537E-4F6C-D104768A1214 for
    emitting notifications or indications. 

    It will wait for BME280 data to be send by the peripheral. But next to that the client
    can send a command / request to the server based on a menu structure.

    It is partly based on
    https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-ble-GattClient/file/71d7cec222eb/main.cpp/

    But has many more features added

    Version 1.0 / January 2022 / paulvha
 */
///////////////////////////////////////////////////////////////////////
//uncomment below to enable MBED-ble trace messages (see SetDebugBLE.txt)
//#define BLE_Debug 1
//////////////////////////////////////////////////////////////////////

#include "gatt_client_process.h"

// event handler
static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

// the Gattdemo constructor
GattClientDemo demo;

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

/* this process will handle basic ble setup and advertising for us */
GattClientProcess ble_process(event_queue, ble_instance);

// Input check in milliseconds
static const std::chrono::milliseconds CHECK_RATE_MS = 450ms;

// name of peer device to look for
char peer_device[20]= "MBED_BME280";

#define BUFLEN 10
char     input[BUFLEN];             // keyboard input buffer
int      inpcnt = 0;                // keyboard input buffer length

/***********************************************************************/
// Define here the structure for the data to exchange.
// This structure must be defined the same on the central and peripheral
// max 20 ( MTU size - 3 header/overhead)
// The Union structure has been choosen to stay within that limit
// Other sketches will provide a structure to send larger data frames
/***********************************************************************/
struct data_to_exchange {
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
  union {
    uint8_t meter;       // if true altitude is in meter, else in feet
    uint8_t CmdStat;     // if MagicNumber then this shows the CMD feedback
  };
  union {
    uint8_t celsius;      // if true temperature is celsius, else fahrenheit
    uint8_t MagicNumber;  // OR magic number MAGIC_CMD
  };
};

#define MAGIC_CMD 0XCF    // the data packet contains command feedback.

/*
 * Commands allowed to send to peripheral. 
 * Must be the same between Central and peripheral
 */
enum key_input{
  REQUEST_METERS = 1,
  REQUEST_FEET,
  REQUEST_CELSIUS, 
  REQUEST_FRHEIT, 
  REQUEST_STOP,
  REQUEST_START,
  REQUEST_NOW,
  REQUEST_SHOW      // local only
};

///////////////////////////////////////////////////////////
// NO CHANGES NEEDED BEYOND THIS POINT
///////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nExample14 GattClient BME280\r");

#ifdef BLE_Debug
    Enable_BLE_Debug(true);
#endif
    
  /* once it's done it will let us continue with our demo */
  ble_process.on_init(mbed::callback(&demo, &GattClientDemo::start));
  ble_process.on_connect(mbed::callback(&demo, &GattClientDemo::start_discovery));
  ble_process.on_disconnect(mbed::callback(&demo, &GattClientDemo::stop));

  // set name of peripheral to look for
  ble_process.set_peer_device_name(peer_device);

  // (optional) add services filter
  // MAXUUID (defined in gattclient.h) can be added (default 5)
  // Set service like: "19B10000-E8F2-537E-4F6C-D104768A1214" or "180F" 
  // NO '0x' in front
  SetFilter(String ("19B10010-E8F2-537E-4F6C-D104768A1214"));

  // determine which characteristic to use for commands to peripheral
  SetCmdChar(String ("19B10011-E8F2-537E-4F6C-D104768A1214"));
  
  // add keyboard check as a recurring event on the event queue
  event_queue.call_every(CHECK_RATE_MS, &Check_loop);    

  printf("Start search for device %s\r\n", peer_device);
  
  ble_process.start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // NOT USED. USE Check_Loop()
}

/* called regular from event queue */
void Check_loop()
{
  static bool ShowOnce = true;
  
  // only show menu after after discovery is complete
  if (ShowOnce){
    // if discover is complete
    if (demo.ConnectionComplete()) {
      printf("Discovery completed\r\n");
      display_menu();
      ShowOnce = false;
    }
  }
  
  // check & handle any keyboard input
  if (Serial.available()) {
    while (Serial.available()) {
      handle_input(Serial.read());
      delay(50);
    }
  }
}

/* handle local keyboard input */
void handle_input(char c)
{
  if (c == '\r') return;    // skip CR
        
  if (c != '\n') {          // act on linefeed
    input[inpcnt++] = c;
    if (inpcnt < BUFLEN -1 ) return; // prefend overflow
  }
  
  input[inpcnt] = 0x0;

  SendCmd((uint8_t) atoi(input));
 
  display_menu();
  
  // reset keyboard buffer
  inpcnt = 0;
}

/* display menu (keep in sync between central and peripheral)*/
void display_menu()
{
  Serial.println(F("1.  Request altitude in meters\r"));
  Serial.println(F("2.  Request altitude in feet\r"));
  Serial.println(F("3.  Request temperature in Celsius\r"));
  Serial.println(F("4.  Request temperature in Fahrenheit\r"));
  Serial.println(F("5.  Request STOP sending data\r"));
  Serial.println(F("6.  Request (re)START sending data\r"));
  Serial.println(F("7.  Request NOW sending data\r"));
  Serial.println(F("    Or just wait\r"));
}

/* display the feedback from server on the sent command */
void DisplayCmdFeedback(uint8_t resp){

 switch (resp)
  {
    case REQUEST_METERS:
        printf("Reporting in meters\r\n");
        break;

    case REQUEST_FEET:
        printf("Reporting in feet\r\n");
        break;

    case REQUEST_CELSIUS:
        printf("Reporting in Celsius\r\n");
        break;

    case REQUEST_FRHEIT:
        printf("Reporting in Fahrenheit\r\n");
        break;

    case REQUEST_STOP:
        printf("Stop sending\r\n");
        break;
    
    case REQUEST_START:
        printf("(re)Start sending\r\n");
        break;
    
    case REQUEST_NOW:   // ignore
    case REQUEST_SHOW:
        break;
        
    default:
        printf("Unknown request.\r\n");
        break;
  }  
}

/* send the command to peripheral/server */
void SendCmd(uint8_t cmd)
{
  if (! demo.WriteCmdChar(&cmd, 1)) {
    printf("Error during writing command\r\n");
  }
}

/* here is the BME280 data OR Command feedback being received handled */
void On_data_received(const GattHVXCallbackParams* event){
 /*
   // debug only
   printf("Data received %u: new value = ", event->handle);
    for (size_t i = 0; i < event->len; ++i) {
        printf("0x%02X ", event->data[i]);
    }
    printf("\r\n");
*/    
    struct data_to_exchange *p = (struct data_to_exchange *) event->data; 

    // in case feedback on earlier sent command
    if (p->MagicNumber == MAGIC_CMD) {
      DisplayCmdFeedback(p->CmdStat);
      return;
    }
    
    Serial.print(F("\r\nTemperature\t"));
    Serial.print(p->temperature);
    if (p->celsius) Serial.println("*C\r");
    else Serial.println(F("*F\r"));
  
    Serial.print(F("Humidity\t"));
    Serial.print(p->humidity);
    Serial.println(F("%\r"));
  
    Serial.print(F("Pressure\t"));
    Serial.print(p->pressure);
    Serial.println(F(" hPa\r"));
  
    Serial.print(F("Altitude\t"));
    Serial.print(p->altitude);
    if (p->meter) Serial.println(F(" Meter\r"));
    else Serial.println(F(" Feet\r"));
}

/* enable Service filter */
void SetFilter(String sfilter)
{
  uint8_t filter[16];
  int k;
  
  // set UUID filter  
  if (sfilter.length() > 0) {
    k = StringToHex(sfilter, &filter[0]);
    if (k > 0) demo.AddServiceFilter(filter, k);
  }
}

/* set the characteristic UUID for sending command to server/peripheral */
void SetCmdChar(String CmdChar)
{
  uint8_t filter[16];
  int k;

  // set UUID filter  
  if (CmdChar.length() > 0) {
    k = StringToHex(CmdChar, &filter[0]);
    if (k > 0) demo.AddCmdChar(filter, k);
  }
}

/** parse a string to hex bytes in reverse order
 *  
 *  @param input : string to parse
 *  @param output : array to store
 *  
 *  return: 
 *    length in bytes
 *    0 = error
 */
int StringToHex(String input, uint8_t *output)
{
  uint8_t i, cnt, h, t;
  uint8_t l = input.length();
  int ret;
  bool Nib_st = true;

  // determine length of input
  if (l == 17) ret = 6;      // address       
  else if (l > 32) ret = 16; // UUID 128 bits
  else if (l > 4) ret = 4;   // UUID 32 bits
  else ret = 2;              // UUID 16 bits
 
  for(i = 0, cnt = 0; i < l; i++) {
    
    h = input[i];
    
    if (h == '-' || h == ':') continue;

    // ascii to hex
    if( h >= '0' && h <= '9') h = h - '0';
    else if( h >= 'a' && h <= 'f') h = h - 'a' + 0xa;
    else h = h - 'A' + 0xa;

    // store in single byte
    if (Nib_st) {
      t = (h & 0xf) << 4; 
      Nib_st = !Nib_st;
    }
    else {
      t |= (h & 0xf); 
      Nib_st = !Nib_st;

      output[cnt++] = t;
    }
  }
   
  if (! Nib_st ) {
    printf("Error during translating \r\n");
    Serial.println(input);
    ret = 0;
  }

  return(ret);
}
