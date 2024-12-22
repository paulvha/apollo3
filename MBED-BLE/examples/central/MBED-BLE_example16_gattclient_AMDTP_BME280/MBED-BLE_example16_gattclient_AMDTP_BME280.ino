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

  It works in conjunction with the peripheral MBED-BLE_example16_gattserv_AMDTP_BME280

  This is a example of using AMD Transfer Protocol (AMDTP) service. With AMDTP you can send larger
  data sizes (by default up to 512 bytes). This data will be broken down by the sender in smaller 
  packages to the size of the MTU. The smaller pacakages will be controlled exchanged between sender and 
  receiver with the use of the AMDTP. On the receiver side these package will be combined 
  into a single data package and the end-to-end CRC will be checked to make sure the data is correct.

  In this example we exchange only a small data package: The size of the BME280 data structure. 
  The example will show a menu and the client can send instructions to the server/peripheral to
  change the data on the server to be send to the client/central.  

  The AMDTP is based on the source code that is provided with AmbiqSuiteSDK. More information is available
  in the extras folder of the library.

  The sketch and connection to the server / peripheral is partly based on
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

// BLE constructor
BLE& ble_instance = ble_instance.Instance();

/* this process will handle basic ble setup and advertising, connecting for us */
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
/***********************************************************************/
struct data_to_exchange {
  // BME280 data
  float humidity;
  float pressure;
  float altitude;
  float temperature;
  uint8_t meter;          // if true altitude is in meter, else in feet
  uint8_t celsius;        // if true temperature is celsius, else fahrenheit
  
  // command feedback
  int8_t CmdStat;          // echo of earlier send commmand or -1 in case of error
  uint8_t MagicNumber;     // magic number
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

bool Sending_in_chunks = false;    // indicate that sending in chunks is in progress
///////////////////////////////////////////////////////////
// NO CHANGES NEEDED BEYOND THIS POINT
///////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(500); }
  Serial.println("\r\nExample16 GattClient AMDTP for BME280\r");

#ifdef BLE_Debug
    Enable_BLE_Debug(true);
#endif

  // set name of peripheral to look for
  ble_process.set_peer_device_name(peer_device); 
  
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

  // connected, all characteristic discovered
  if (ble_process.IsConnectionComplete()) {
    
    // only show menu after discovery is complete
    if (ShowOnce){
      printf("Discovery completed\r\n");
      display_menu();
      ShowOnce = false;
    }
  }
  else
    ShowOnce = true;
  
  // check & handle any keyboard input
  if (Serial.available()) {
    while (Serial.available()) {
      handle_input(Serial.read());
      delay(50);
    }
  }

    // if sending in chunk is in progress
  if (Sending_in_chunks) {
    if (ble_process.IsSendingComplete()){
      printf("All has been sent \r\n");
      Sending_in_chunks = false;
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
  
  // reset local keyboard buffer
  inpcnt = 0;
}

/* display menu (keep in sync between central and peripheral)*/
void display_menu()
{
  Serial.println();
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
  uint8_t nn = cmd;
  
  int ret = ! ble_process.SendToServer(&nn, 1);
  
  if (ret == -1) {
    printf("Error during sending echo\r\n");
  }
  else if (ret == 1) {
    printf("Sending back in different packages. Wait to complete\r\n");
    Sending_in_chunks = true;
  }
}

/* here is the BME280 data OR Command feedback being received handled */
void StoreDataReceived(uint8_t *data, uint16_t len){
 /*
   // debug only
    for (uint16_t i = 0; i < len; ++i) {
        printf("0x%02X ", data[i]);
    }
    printf("\r\n");
*/    
    struct data_to_exchange *p = (struct data_to_exchange *) data; 

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
