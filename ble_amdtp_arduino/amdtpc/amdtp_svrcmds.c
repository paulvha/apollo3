// ****************************************************************************
//
//
//! @file  amdtp_srvcmds.c
//!
//! @brief This file provides the client user interface for the amdtc service
//!
//! This has been created and tested to work against Apollo3-board running ble_amdts.ino
//!
//! paulvha / March 2020 / version 1.0
//! @{
//
// ****************************************************************************
//*****************************************************************************
//
// Copyright (c) 2020, Paul van Haastrecht
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include "amdtp_svrcmds.h"

// enable debug for this file
// AM_DEBUG_PRINTF is defined in ble_menu.h
#ifdef AM_DEBUG_PRINTF
#define AMDTP_SRVCMD_DEBUG
#endif

// stages for measurering with load resistor
uint8_t b_load = 0;

// waiting on additional command for server?
eAmdtpPktcmd_t PendingCmdInput = AMDTP_CMD_NONE;

// continued testdata
bool ContTestData = false;

//*****************************************************************************
//
// check for valid digital pin
// Return : TRUE for OK else FALSE
//
//*****************************************************************************
bool validate_pin(int pin)
{
  if(pin >= 1 && pin <= 49) {
      if(pin != 30 && pin != 46) return TRUE;
  }

  return FALSE;
}

//*****************************************************************************
//
// handle waited for additional input for server
//
//*****************************************************************************
void CmdInput()
{
  switch (PendingCmdInput){
    case AMDTP_CMD_READ_PIN:
    case AMDTP_CMD_PIN_HIGH:
    case AMDTP_CMD_PIN_LOW:
    case AMDTP_CMD_ADC:
        SendReqPin();
        PendingCmdInput = AMDTP_CMD_NONE;
        break;
    case AMDTP_CMD_CHAT:
        SendChat();
        break;
    default:
        am_menu_printf("Unknown Command\r\n");
        PendingCmdInput = AMDTP_CMD_NONE;
      break;
  }
}

//*****************************************************************************
//
// Handle keyboard command input
//
//*****************************************************************************
void handleAMDTPSlection(void)
{
  uint8_t id;
  id = (uint8_t)atoi((char *) menuRxData);

  switch (id)
  {
    case 1:
        am_menu_printf("Request server to send\r\n");
        ContTestData = true;
        AmdtpcSendCmd(AMDTP_CMD_START_TEST_DATA, NULL, 0);
        break;
    case 2:
        am_menu_printf("request server to stop\r\n");
        ContTestData = false;
        break;
    case 3:
        am_menu_printf("Sent Hello to server\r\n");
        AmdtpcSendCmd(AMDTP_CMD_HELLO, NULL, 0);
        break;
    case 4:
        am_menu_printf("Request battery percentage level\r\n");
        AmdtpcSendCmd(AMDTP_CMD_REQ_BATTERY_LEVEL, NULL, 0);
        break;
    case 5:
        am_menu_printf("Request battery level percentage with load resistor\r\n");
        b_load = 1;
        AmdtpReqBatLd();
        break;
    case 6:
        am_menu_printf("Request internal temperature in Celsius\r\n");
        AmdtpcSendCmd(AMDTP_CMD_REQ_INTERNAL_TEMP_CEL, NULL, 0);
        break;
    case 7:
        am_menu_printf("Request internal temperature in Fahrenheit\r\n");
        AmdtpcSendCmd(AMDTP_CMD_REQ_INTERNAL_TEMP_FRH, NULL, 0);
        break;
    case 8:
        am_menu_printf("Turn server onboard led on\r\n");
        AmdtpcSendCmd(AMDTP_CMD_TURN_LED_ON, NULL, 0);
        break;
    case 9:
        am_menu_printf("Turn server onboard led off\r\n");
        AmdtpcSendCmd(AMDTP_CMD_TURN_LED_OFF, NULL, 0);
        break;
    case 10:
        am_menu_printf("Request BME280 measurement\r\n");
        AmdtpcSendCmd(AMDTP_CMD_BME280, NULL, 0);
        break;
    case 11:
        am_menu_printf("Request ADC pin level\r\n");
        AmdtpAskPinInput(AMDTP_CMD_ADC);
        break;
    case 12:
        am_menu_printf("Start chat session. Please enter your text >>>>> ");
        PendingCmdInput = AMDTP_CMD_CHAT;
        break;
    case 13:
        am_menu_printf("Request digital pin level\r\n");
        AmdtpAskPinInput(AMDTP_CMD_READ_PIN);
        break;
    case 14:
        am_menu_printf("Set digital pin High\r\n");
        AmdtpAskPinInput(AMDTP_CMD_PIN_HIGH);
        break;
    case 15:
        am_menu_printf("Set digital pin Low\r\n");
        AmdtpAskPinInput(AMDTP_CMD_PIN_LOW);
        break;
   
    case 30:
        am_menu_printf("Request server version\r\n");
        AmdtpcSendCmd(AMDTP_CMD_VERSION, NULL, 0);
        break;

    default:
        am_menu_printf("Unknown request, %d\r\n", id);
        break;
  }
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
    AmdtpcSendCmd(AMDTP_CMD_REQ_BATTERYLOAD_ON, NULL,0);
    b_load++;
  }
  // now measure
  else if (b_load == 2){
    AmdtpcSendCmd(AMDTP_CMD_REQ_BATTERY_LEVEL, NULL,0);
    b_load++;
  }
  // turn off the battery load
  else {
    AmdtpcSendCmd(AMDTP_CMD_REQ_BATTERYLOAD_OFF, NULL,0);
    b_load = 0;
  }
}

//*****************************************************************************
//
// Ask for digital or analog pin
//
//*****************************************************************************
void AmdtpAskPinInput(uint8_t cmd)
{
  if (cmd == AMDTP_CMD_ADC)
    am_menu_printf("Enter ADC PIN (0 = cancel)\r\n");
  else
    am_menu_printf("Enter Digital PIN (0 = cancel)\r\n");

  // set for waiting command input
  PendingCmdInput = cmd;
}

//*****************************************************************************
//
// Send request for Analog or digital pin to server
//
//*****************************************************************************
void SendReqPin()
{
  bool correct = TRUE;
  uint8_t pin = (uint8_t) atoi(menuRxData);

  if (pin == 0) {
    am_menu_printf("Cancel selected\r\n");
    return;
  }

  // ADC pin will be validated on server given
  // different pad to pin mapping on different boards
  if (PendingCmdInput != AMDTP_CMD_ADC)
    correct = validate_pin(pin);

  if (correct) {
    AmdtpcSendCmd(PendingCmdInput, &pin, 1);
  }
  else {
    am_menu_printf("Invalid pin %d\r\n", pin);
    BleMenuShowMenu();
  }
}

//*****************************************************************************
//
// chat with server
//
// get a line from the client and sent to the server
//
// Chat is terminated by typing "BYE". It will return FALSE in that case
//
//*****************************************************************************

void SendChat()
{
  am_menu_printf("%s\r\n", menuRxData);

  // check for stopping chat
  if (menuRxDataLen == 3) {
    if (strcmp(menuRxData,"BYE") == 0){
      AmdtpcSendCmd(AMDTP_CMD_STOP_TEST_DATA, NULL, 0);
      return;
    }
  }

  // sent to server
  AmdtpcSendCmd(AMDTP_CMD_CHAT, (uint8_t *) menuRxData, menuRxDataLen+1);
}

//*****************************************************************************
//
// Sent command to server
// param cmd : server command to sent
// param buf : buffer with optional data
// param len : length of optional data
//
// Return : AMDTP_STATUS_SUCCESS = Ok, else error
//
//*****************************************************************************
eAmdtpStatus_t AmdtpcSendCmd(uint8_t cmd, uint8_t *buf, uint8_t len)
{
  uint8_t data[RXDATALEN] = {0};
  eAmdtpStatus_t status;

  data[0] = cmd;

  if (len > 0)
  {
    if (len > RXDATALEN)  return(AMDTP_STATUS_INSUFFICIENT_BUFFER);
    memcpy(data + 1, buf, len);
  }

#ifdef AMDTP_SRVCMD_DEBUG
    am_menu_printf("%s calling AmdtpcSendPacket... \r\n",__func__);
#endif

  //                          data packet,   encrypted, enableACK, data, length of data
  status = AmdtpcSendPacket(AMDTP_PKT_TYPE_DATA, false, true, data, len + 1);

#ifdef AMDTP_SRVCMD_DEBUG
    am_menu_printf("AmdtpcSendPacket = %d, Opcode Sent = %d\r\n", status, data[0]);
#endif

  if (status != AMDTP_STATUS_SUCCESS)
  {
    am_menu_printf("Error...did not receive AMDTP_STATUS_SUCCESS from AmdtpcSendPacket \r\n");
  }

  return(status);
}

//*****************************************************************************
//
// handle responds received from the server
//
//*****************************************************************************
void HandeServerResp(uint8_t * buf, uint16_t len)
{
  uint16_t val;
  int16_t ival;
  bool showmenu = false;

#ifdef AMDTP_SRVCMD_DEBUG
  uint16_t i;
  am_menu_printf("%d bytes data received : ", len);
  for (i = 0; i < len; i++)  am_menu_printf("0x%X ", buf[i]);
  am_menu_printf("\r\n");
#endif

  switch (buf[0]) {

    case AMDTP_CMD_HELLO:
        am_menu_printf("\r\nReceived HELLO responds\r\n");
        showmenu = true;
        break;

    case AMDTP_CMD_TURN_LED_ON:
        am_menu_printf("\r\nTurned server Led on\r\n");
        showmenu = true;
        break;

    case AMDTP_CMD_TURN_LED_OFF:
        am_menu_printf("\r\nTurned server Led off\r\n");
        showmenu = true;
        break;

    case AMDTP_CMD_REQ_BATTERYLOAD_ON:
        am_menu_printf("\r\nTurned load resistor ON\r\n");
        if (b_load == 2) AmdtpReqBatLd();
        else showmenu = true;
        break;

    case AMDTP_CMD_REQ_BATTERYLOAD_OFF:
        am_menu_printf("\r\nTurned load resistor OFF\r\n");
        showmenu = true;
        break;

    case AMDTP_CMD_REQ_BATTERY_LEVEL:
        am_menu_printf("\r\nInternal Battery level is at %2.2f %%\r\n", byte_to_float(buf, 2));

        // measurement with resistor load
        if (b_load == 3) AmdtpReqBatLd();
        else showmenu = true;
        break;

    case AMDTP_CMD_REQ_INTERNAL_TEMP_FRH:

        am_menu_printf("\r\nInternal temperature is %2.2f F\r\n", byte_to_float(buf, 2));
        showmenu = true;
        break;

    case AMDTP_CMD_REQ_INTERNAL_TEMP_CEL:

        val = buf[2] << 8 | buf[3];
        am_menu_printf("\r\nInternal temperature is %2.2f C\r\n", byte_to_float(buf, 2));
        showmenu = true;
        break;

    case AMDTP_CMD_BME280:
        am_menu_printf("\r\nRead BME280\r\n");
        display_BME280(buf, len);
        showmenu = true;
        break;

    case AMDTP_CMD_START_TEST_DATA:

        val = buf[len-2] << 8 | buf[len - 1];
        am_menu_printf("Test data : %s %d\r\n", &buf[2], val);

        if (ContTestData)
          AmdtpcSendCmd(AMDTP_CMD_START_TEST_DATA, NULL, 0);
        else
          AmdtpcSendCmd(AMDTP_CMD_STOP_TEST_DATA, NULL, 0);
        break;

    case AMDTP_CMD_STOP_TEST_DATA:

        am_menu_printf("\r\nStopped test data\r\n");
        PendingCmdInput = AMDTP_CMD_NONE;
        showmenu = true;
        break;

    case AMDTP_CMD_ADC:

        ival = buf[3] << 8 | buf[4];

        if(ival == -1){
            am_menu_printf("%d : INCORRECT analogPin\n", buf[2]);
        }
        else {
            am_menu_printf("\r\nADC value channel %d is %d or %2.1f volt\r\n", buf[2], ival, (float) ((float)ival * 3.3F / 1023));
        }
        showmenu = true;
        break;

    case AMDTP_CMD_CHAT:

        am_menu_printf("<<<<< %s\r\n", &buf[2]);

        // if receiving BYE stop chatting
        if (len == 6){
          if (strcmp((char *) &buf[2],"BYE") == 0) PendingCmdInput = AMDTP_CMD_NONE;
          showmenu = true;
        }
        else
          am_menu_printf(">>>>> ");
        break;

    case AMDTP_CMD_READ_PIN:

        am_menu_printf("\r\nDigital pin %d is reading %s\r\n", buf[2], buf[3]?"HIGH":"LOW");
        showmenu = true;
        break;

    case AMDTP_CMD_PIN_HIGH:

        am_menu_printf("\r\nDigital pin %d is set high\r\n", buf[2]);
        showmenu = true;
        break;

    case AMDTP_CMD_PIN_LOW:

        am_menu_printf("\r\nDigital pin %d is set low\r\n", buf[2]);
        showmenu = true;
        break;

    case AMDTP_CMD_VERSION:     // request server version
        if (buf[2] != MAJOR_CLIENTVERSION){
           am_menu_printf("*****************************************\r\n");
           am_menu_printf("** WARNING MAJOR VERSIONS DO NOT MATCH **\r\n");
           am_menu_printf("**    not all features supported !!    **\r\n");
           am_menu_printf("*****************************************\r\n");
        }
        am_menu_printf("\r\nServer version: %d.%d", buf[2], buf[3]);
        am_menu_printf("\r\nClient version: %d.%d\r\n",MAJOR_CLIENTVERSION, MINOR_CLIENTVERSION);
        showmenu = true;
        break;
        
    case AMDTP_CMD_CUSTOM1:      // repeat for other options
        am_menu_printf("\r\nAMDTP_CMD_CUSTOM1\r\n");

        showmenu = true;
        // Do something here....
        break;

    default:

        am_menu_printf("\r\nUNKNOWN request / answer returned : 0x%X\r\n",buf[0]);
        showmenu = true;
        break;
  }

  if (showmenu) {
    am_menu_printf("\r\n");
    BleMenuShowMenu();
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
      am_menu_printf("BME280 was not detected or not implemented on server\r\n");
      return;
  }

  if (buf[1] != 18 ) {
      am_menu_printf("Not enough data received. Expected 18 bytes, got %d\r\n", buf[1]);
      return;
  }

  // get temperature
  ind = buf[x++];
  ret = byte_to_float(buf, x);
  am_menu_printf("\r\nTemperature\t%2.2f %c\r\n", ret, ind);
  x += 4;

  // get humidity
  ret = byte_to_float(buf, x);
  am_menu_printf("Humidity\t%2.2f %%\r\n", ret);
  x += 4;

   // get pressure
  ret = byte_to_float(buf, x);
  am_menu_printf("Pressure\t%2.2f hPa\r\n", ret/100);
  x += 4;

  // get altitude
  ind = buf[x++];
  ret = byte_to_float(buf, x);
  am_menu_printf("Altitude\t%2.2f %c\r\n", ret, ind);
}
