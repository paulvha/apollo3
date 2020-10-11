/*
  BLE <-> Arduino client server comm.  amdtp_CLIENT version derived from ble_freertos_amdtpc SDK app.

  This example demonstrates basic BLE server (pheripheral) functionality for the Apollo3 boards.

  How to use this example:

  Make sure to load on another Apollo3 board the amdtp_server sketch.
  See the amdtp_arduino.odt for more information
    
  Start the server
  Start the client
  
  Version 2.0 / March 2020
  * updated some typos
  * SmpHandlerInit() update
  * changed ADC pin validation to server
  * added major / minor number
  * added requesting server version numbers
  * update to capture friendly name

 Version 2.0.1 / April 2020
  * fixed issue with Bluetooth friendly names

 Version 2.0.2 / April 2020
  * fixed issue with look-up friendly name (provided by Randy Lewis)

 Version 3.0 / October 2020
  *  major rewrite as the amdtp_client is now working on top of ArduinoBLE.h 
*/
/********************************************************************************************************************
 *******************************************************************************************************************/

// version number
#define MAJOR_CLIENTVERSION 3     // new features
#define MINOR_CLIENTVERSION 0     // bug fixes, better calculation/ layout

//*********************************************************************
//  NO CHANGES NEEDED BEYOND THIS POINT
//*********************************************************************

#include "amdtp_common.h"
#include "amdtp_bridge.h"

char input[50];                   // keyboard input buffer
int inpcnt = 0;                   // keyboard input buffer length
int PendingCmdInput = 0;          // chat of request for pin
uint8_t b_load = 0;               // measure battery with load resistor
uint8_t receiveBuf[MAXRECEIVEBUF];// hand off data buffer from Bluetooth to user level
uint16_t ReceiveBufLen;           // length of data in buffer
bool RespReceive = false;         // indicator new data in buffer
bool ContTestData = false;        // indicator continues testdata

void setup() {

  Serial.begin(115200);
  Serial.printf("\rArduino AMDTP client. Version : %d.%d\r\n", MAJOR_CLIENTVERSION, MINOR_CLIENTVERSION);
 
  // connect to server
  perform_connecting();
  
  // initialize AMDTP
  amdtps_init();
  
  clr_input();
  display_menu();
}

void loop() {
  
  // poll and check connect
  if (!CheckConnect()) {
    Serial.println(F("\rDisconnected"));
    Serial.println(F("\r\nPush RESET-button on client to reconnect")); 
    while(1);
  }

  // handle any keyboard input
  if (Serial.available()) {
    while (Serial.available()) handle_input(Serial.read());
  }
  
  // handle any new data received over BlueTooth
  if (RespReceive) {
    DisplayServerResp();
  }
}

//***************************************************************
//*
//* Connect to AMDTP server.
//*
//***************************************************************
void perform_connecting()
{
  Serial.println(F("\rStarting Bluetooth"));
    
  // Start ArduinoBLE
  if (!StartBLE()){
    Serial.println(F("\rStarting BLE failed!"));
    while(1);
  }
  
  // find AMDTP server
  if (!PerformScan()){
    Serial.println(F("\rCould not find AMDTP server"));
    while(1);
  }

  // connect and find handles
  if (!get_handles()){
    Serial.println(F("\rCould not connect or find all characteristics"));
    while(1);
  }
}

void display_menu()
{
  Serial.println("\r1.  Request Server to send test data ");
  Serial.println("\r2.  Request Server to stop sending test data");
  Serial.println("\r3.  Sent 'Hello' to server");
  Serial.println("\r4.  Request battery level percentage");
  Serial.println("\r5.  Request battery level percentage with load resistor");
  Serial.println("\r6.  Request internal temperature in Celsius");
  Serial.println("\r7.  Request internal temperature in Fahrenheit");
  Serial.println("\r8.  Turn server onboard led on");
  Serial.println("\r9.  Turn server onboard led off");
  Serial.println("\r10. Request BME280 measurement");
  Serial.println("\r11. Request ADC pin level");
  Serial.println("\r12. Request chat session");
  Serial.println("\r13. Request digital pin level");
  Serial.println("\r14. Set digital pin High");
  Serial.println("\r15. Set digital pin Low");
  Serial.println("\r20. Disconnect from server");
  Serial.println("\r30. Request server version number");
};

//***************************************************************
//*
//* handle keyboard input 
//*
//***************************************************************

void handle_input(char c)
{
  if (c == '\r') return;    // skip CR
        
  if (c != '\n') {          // act on linefeed
    input[inpcnt++] = c;
    return;
  }
  
  input[inpcnt] = 0x0;
  int req = atoi(input);

  // Waiting for pin or chat
  if (PendingCmdInput) {
    if (PendingCmdInput == AMDTP_CMD_CHAT)    SendChat();
    else handle_pin(req);
    return;
  }
  
  switch (req)
  {
    case 1:
        Serial.print("\rRequest server to send test data\r\n");
        ContTestData = true;
        SendCmd(AMDTP_CMD_START_TEST_DATA, NULL, 0);
        break;
    case 2:
        Serial.print("\r*********** Request server to stop sending test data\r\n");
        ContTestData = false;
        break;
    case 3:
        Serial.print("\rSent Hello to server\r\n");
        SendCmd(AMDTP_CMD_HELLO, NULL, 0);
        break;
    case 4:
        Serial.print("\rRequest battery percentage level\r\n");
        SendCmd(AMDTP_CMD_REQ_BATTERY_LEVEL, NULL, 0);
        break;
    case 5:
        Serial.print("\rRequest battery percentage with load resistor\r\n");
        b_load = 1;
        AmdtpReqBatLd();
        break;
    case 6:
        Serial.print("\rRequest internal temperature in Celsius\r\n");
        AmdtpcSendCmd(AMDTP_CMD_REQ_INTERNAL_TEMP_CEL, NULL, 0);
        break;
    case 7:
        Serial.print("\rRequest internal temperature in Fahrenheit\r\n");
        SendCmd(AMDTP_CMD_REQ_INTERNAL_TEMP_FRH, NULL, 0);
        break;
    case 8:
        Serial.print("Turn server onboard led on\r\n");
        SendCmd(AMDTP_CMD_TURN_LED_ON, NULL, 0);
        break;
    case 9:
        Serial.print("\rTurn server onboard led off\r\n");
        AmdtpcSendCmd(AMDTP_CMD_TURN_LED_OFF, NULL, 0);
        break;
    case 10:
        Serial.print("\rRequest BME280 measurement\r\n");
        SendCmd(AMDTP_CMD_BME280, NULL, 0);
        break;
    case 11:
        Serial.print("Request ADC pin level\r\n");
        ask_pin_input(AMDTP_CMD_ADC);
        break;
    case 12:
        Serial.print("\rStart chat session. Please enter your text >>>>> ");
        PendingCmdInput = AMDTP_CMD_CHAT;
        break;
    case 13:
        Serial.print("\rRequest digital pin level\r\n");
        ask_pin_input(AMDTP_CMD_READ_PIN);
        break;
    case 14:
        Serial.print("\rSet digital pin High\r\n");
        ask_pin_input(AMDTP_CMD_PIN_HIGH);
        break;
    case 15:
        Serial.print("\rSet digital pin Low\r\n");
        ask_pin_input(AMDTP_CMD_PIN_LOW);
        break;
    case 20:
        Serial.print("\rRequest to disconnect\n");
        perform_disconnect();
        break;
    case 30:
        Serial.print("\rRequest server version\r\n");
        SendCmd(AMDTP_CMD_VERSION, NULL, 0);
        break;
    default:
        Serial.printf("\rUnknown request, %d\r\n", req);
        display_menu();
        break;
  }

  // reset keyboard buffer
  inpcnt = 0;
}

//*****************************************************************************
//
// Clear input parameters
//
//*****************************************************************************
void clr_input()
{
  PendingCmdInput = 0;
  input[0] = 0x0;
  inpcnt = 0;
}

//*****************************************************************************
//
// Sent command to server
//
//*****************************************************************************
void SendCmd(uint8_t cmd, uint8_t *buf, uint8_t len)
{
  if (! AmdtpcSendCmd(cmd, buf, len) ){
    Serial.printf("\rError during sending command\n");
    clr_input();
  }
}

//*****************************************************************************
//
// for commands that need a PIN input
//
//****************************************************************************
void ask_pin_input(int cmd)
{
  PendingCmdInput = cmd;

  if (cmd == AMDTP_CMD_ADC)
    Serial.println("\rEnter ADC PIN (0 = cancel)");
  else
    Serial.println("\rEnter Digital PIN (0 = cancel)");
}

//*****************************************************************************
//
// pin input received now sent to server
//
//*****************************************************************************
void handle_pin (int pin)
{
  bool correct = true;
  
  if (pin == 0) {
     clr_input();
     display_menu();
     return;
  }
 
  // ADC pin will be validated on server given
  // different pad-to-pin mapping on the different boards
  if (PendingCmdInput != AMDTP_CMD_ADC){
     
    if (pin >= 1 && pin <= 49 && pin != 30 && pin != 46 )   correct = true;
    else  correct = false;
  }

  if (correct) 
    SendCmd(PendingCmdInput, (uint8_t *) &pin, 1);
  else {
    Serial.printf("\rInvalid pin %d\n", pin);
    display_menu();
  }

  clr_input();
}

//*****************************************************************************
//
// chat with server
//
// get a line from the client and sent to the server
//
// Chat is terminated by typing "BYE".
//
//*****************************************************************************
void SendChat()
{
  Serial.printf("\r%s\n", input);

  // check for stopping chat
  if (inpcnt == 3) {
    
    if (strcmp(input,"BYE") == 0){
      SendCmd(AMDTP_CMD_STOP_TEST_DATA, NULL, 0);
      clr_input();
      display_menu();
      return;
    }
  }

  // sent to server
  SendCmd(AMDTP_CMD_CHAT, (uint8_t *) input, inpcnt+1);
  inpcnt = 0;
}

//*****************************************************************************
//
// send command to read the battery level with load resistor
//
//*****************************************************************************
void AmdtpReqBatLd()
{
  // turn on the battery load
  if (b_load == 1){
    SendCmd(AMDTP_CMD_REQ_BATTERYLOAD_ON, NULL,0);
    b_load++;
  }
  // now measure
  else if (b_load == 2){
    SendCmd(AMDTP_CMD_REQ_BATTERY_LEVEL, NULL,0);
    b_load++;
  }
  // turn off the battery load
  else {
    SendCmd(AMDTP_CMD_REQ_BATTERYLOAD_OFF, NULL,0);
    b_load = 0;
  }
}

//*****************************************************************************
//
// handle responds received from the server
// 
// Hand-off the data and let the "stack" clear with all the returns pending
// to ArduinoBLE and Amdtp functions to prevent stack-overrun. 
// The display of the data is triggered once returned in loop()
//*****************************************************************************
void HandleServerResp(uint8_t * buf, uint16_t len)
{
  for (ReceiveBufLen = 0 ; ReceiveBufLen < len ; ReceiveBufLen++) receiveBuf[ReceiveBufLen] = buf[ReceiveBufLen];
  RespReceive = true;
}

//*****************************************************************************
//
// Display responds received from the server
//
//*****************************************************************************
void DisplayServerResp()
{  
  uint16_t val;
  int16_t ival;
  bool showmenu = true;
  
  RespReceive = false;

#ifdef BLE_SHOW_DATA
  uint16_t i;
  Serial.printf("\r%d bytes data received : ", ReceiveBufLen);
  for (i = 0; i < ReceiveBufLen; i++)  Serial.printf("0x%X ", receiveBuf[i]);
  Serial.println();
#endif

  switch (receiveBuf[0]) {

    case AMDTP_CMD_HELLO:
        Serial.printf("\r\nReceived HELLO responds\n");
        break;

    case AMDTP_CMD_TURN_LED_ON:
        Serial.println("\r\nTurned server Led on");
        break;

    case AMDTP_CMD_TURN_LED_OFF:
        Serial.printf("\r\nTurned server Led off\n");
        break;

    case AMDTP_CMD_REQ_BATTERYLOAD_ON:
        Serial.printf("\r\nTurned load resistor ON\n");
        if (b_load == 2) { AmdtpReqBatLd(); showmenu = false;}
        break;

    case AMDTP_CMD_REQ_BATTERYLOAD_OFF:
        Serial.printf("\r\nTurned load resistor OFF\n");
        break;

    case AMDTP_CMD_REQ_BATTERY_LEVEL:
        Serial.printf("\r\nInternal Battery level is at ");
        Serial.print(byte_to_float(receiveBuf, 2));
        Serial.println("%");
        
        // measurement with resistor load (turn it off now)
        if (b_load == 3) { AmdtpReqBatLd(); showmenu = false;}
        break;

    case AMDTP_CMD_REQ_INTERNAL_TEMP_FRH:
        Serial.printf("\r\nInternal temperature is at ");
        Serial.print(byte_to_float(receiveBuf, 2));
        Serial.println(" F");
        break;

    case AMDTP_CMD_REQ_INTERNAL_TEMP_CEL:

        val = receiveBuf[2] << 8 | receiveBuf[3];
        Serial.printf("\r\nInternal temperature is at ");
        Serial.print(byte_to_float(receiveBuf, 2));
        Serial.println(" C");
        break;

    case AMDTP_CMD_BME280:
        Serial.printf("\r\nRead BME280\r\n");
        display_BME280(receiveBuf, ReceiveBufLen);
        break;

    case AMDTP_CMD_START_TEST_DATA:

        val = receiveBuf[ReceiveBufLen-2] << 8 | receiveBuf[ReceiveBufLen - 1];
        Serial.printf("Test data : %s %d\n", &receiveBuf[2], val);
        showmenu = false;
                
        if (ContTestData){
          delay(2000);    // wait 2 seconds
          AmdtpcSendCmd(AMDTP_CMD_START_TEST_DATA, NULL, 0);
        }
        else
          AmdtpcSendCmd(AMDTP_CMD_STOP_TEST_DATA, NULL, 0);
        break;

    case AMDTP_CMD_STOP_TEST_DATA:
        Serial.printf("\r\nStopped test data\n");
        break;

    case AMDTP_CMD_ADC:

        ival = receiveBuf[3] << 8 | receiveBuf[4];

        if(ival == -1){
            Serial.printf("%d : INCORRECT analogPin\n", receiveBuf[2]);
        }
        else {
          Serial.printf("\r\nADC value channel %d is %d or ", receiveBuf[2], ival);
          Serial.print((float) ((float)ival * 3.3F / 1023));
          Serial.println(" volt");
        }
        break;

    case AMDTP_CMD_CHAT:

        Serial.printf("\r<<<<< %s\n", &receiveBuf[2]);

        // if receiving BYE stop chatting
        if (ReceiveBufLen == 6){
          if (strcmp((char *) &receiveBuf[2],"BYE") == 0) PendingCmdInput = AMDTP_CMD_NONE;
        }
        else {
          Serial.printf("\r>>>>> ");
          showmenu = false;
        }
        break;

    case AMDTP_CMD_READ_PIN:

        Serial.printf("\r\nDigital pin %d is reading %s\n", receiveBuf[2], receiveBuf[3]?"HIGH":"LOW");
        break;

    case AMDTP_CMD_PIN_HIGH:

        Serial.printf("\r\nDigital pin %d is set high\n", receiveBuf[2]);
        break;

    case AMDTP_CMD_PIN_LOW:

        Serial.printf("\r\nDigital pin %d is set low\r\n", receiveBuf[2]);
        break;

    case AMDTP_CMD_VERSION:     // request server version
    
        if (receiveBuf[2] != MAJOR_CLIENTVERSION) {
           Serial.printf("*****************************************\r\n");
           Serial.printf("** WARNING MAJOR VERSIONS DO NOT MATCH **\r\n");
           Serial.printf("**    not all features supported !!    **\r\n");
           Serial.printf("*****************************************\r\n");
        }
        Serial.printf("\r\nServer version: %d.%d", receiveBuf[2], receiveBuf[3]);
        Serial.printf("\r\nClient version: %d.%d\r\n",MAJOR_CLIENTVERSION, MINOR_CLIENTVERSION);
        break;
        
    case AMDTP_CMD_CUSTOM1:      // repeat for other options
    
        Serial.printf("\r\nAMDTP_CMD_CUSTOM1\r\n");

        // Do something here....
        break;

    default:

        Serial.printf("\r\nUNKNOWN request / answer returned : 0x%X\r\n",receiveBuf[0]);
        break;
  }

  if (showmenu) {
      clr_input();
      display_menu();
  }
}

//*****************************************************************************
//
// translate 4 bytes to float IEEE754
// param x : offset in received data
//
// return : float number
//
//*****************************************************************************
float byte_to_float(uint8_t *buf, int x)
{
  ByteToFloat conv;
  uint8_t i;
  for (i = 0; i < 4; i++)  conv.array[3-i] = buf[x+i];
  return conv.value;
}

//*****************************************************************************
//
// translate display BME280.
//
// The buffer format is:
// 1 byte  original request
// 1 byte  len of data (it 0 BME280 not enabled or NOT detected on server else 18)
// 4 bytes float humidity
// 4 bytes float pressure
// 1 byte  indicator : M (meter) or F (feet). This is determined in sketch.
// 4 bytes float altitude
// 1 byte  indicator : C (Celsius) or F (Fahrenheit). This is determined in sketch.
// 4 bytes float temperature
//
//*****************************************************************************
void display_BME280(uint8_t *buf, uint16_t len)
{
  float ret;
  uint8_t x = 2 ;     // skip orginal request + len
  char ind;

  if (buf[1] == 0 ) {
      Serial.printf("BME280 was not detected or not implemented on server\r\n");
      return;
  }

  if (buf[1] != 18 ) {
      Serial.printf("Not enough data received. Expected 18 bytes, got %d\r\n", buf[1]);
      return;
  }

  // get temperature
  ind = buf[x++];
  ret = byte_to_float(buf, x);
  Serial.print("\r\nTemperature\t");
  Serial.print(ret);
  Serial.println(ind);
  x += 4;

  // get humidity
  ret = byte_to_float(buf, x);
  Serial.print("Humidity\t");
  Serial.print(ret);
  Serial.println("%");

  x += 4;

   // get pressure
  ret = byte_to_float(buf, x);
  Serial.print("Pressure\t");
  Serial.print(ret/100);
  Serial.println(" hPa");
  x += 4;

  // get altitude
  ind = buf[x++];
  ret = byte_to_float(buf, x);
  Serial.print("Altitude\t");
  Serial.print(ret);
  Serial.println(ind);
}
