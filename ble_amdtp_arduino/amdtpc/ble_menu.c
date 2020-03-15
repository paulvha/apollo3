// ****************************************************************************
//
//  ble_menu.c
//! @file
//!
//! @brief Functions for BLE control menu.
//!
//! @{
//
// ****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro
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
// This is part of revision 2.4.2 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include "ble_menu.h"
#include "wsf_types.h"
#include "amdtp_api.h"
#include "app_api.h"
#include "amdtp_svrcmds.h"

char menuRxData[RXDATALEN];
uint8_t menuRxDataLen = 0;

// keep track of menu to display
sBleMenuCb bleMenuCb;

char mainMenuContent[BLE_MENU_ID_MAX][32] = {
    "1. BLE_MENU_ID_GAP",
    "2. BLE_MENU_ID_GATT",
    "3. BLE_MENU_ID_AMDTP",
};

char gapMenuContent[GAP_MENU_ID_MAX][40] = {
    "1. Start Scan",
    "2. Stop Scan",
    "3. Show Scan Results",
    "4. Create Connection",
    "5. Close connection",
    "6. Toggle through-put measurement",
};

char gattMenuContent[GATT_MENU_ID_MAX][32] = {
    "1. TBD",
};
#define AMDTP_MENU_MAX_ENTRIES 64
char amdtpMenuContent[AMDTP_MENU_MAX_ENTRIES][64] = {
    "1.  Request Server to send test data ",
    "2.  Request Server to stop sending test data",
    "3.  Sent 'Hello' to server",
    "4.  Request battery level percentage",
    "5.  Request battery level percentage with load resistor",
    "6.  Request internal temperature in Celsius",
    "7.  Request internal temperature in Fahrenheit",
    "8.  Turn server onboard led on",
    "9.  Turn server onboard led off",
    "10. Request BME280 measurement",
    "11. Request ADC pin level",
    "12. Request chat session",
    "13. Request digital pin level",
    "14. Set digital pin High",
    "15. Set digital pin Low",
    ""
};

//*****************************************************************************
//
// Keyboard / Menu input handler.
//
//*****************************************************************************
void add_menu_input(char rxData)
{
  if (rxData == '\n' || rxData == '\r') {

    if (menuRxDataLen == 0) return;
    
InputComplete:
    // terminate keyboard input
    menuRxData[menuRxDataLen] = '\0';
    
    // Not waiting for additional info for server cmd
    if (PendingCmdInput == AMDTP_CMD_NONE){
      
      // Menu commands not notifications
      if (menuRxData[0] < NOT_CONN_OPENED ) {
        am_menu_printf("BleMenuRx data = %s\n", menuRxData);
        handleSelection();
      }
      else {
        handleNotification();
      }
    }
    else {  // sent to handle additional command input
      CmdInput();
    }
  
    // reset input buffer
    menuRxDataLen = 0;
  }
  else
  {
    // add to buffer
    menuRxData[menuRxDataLen++] = rxData;

    // prevent keyboard buffer overrun
    if (menuRxDataLen == RXDATALEN) goto InputComplete;
  }
}

//*****************************************************************************
//
// set settings for home
//
//*****************************************************************************
static bool isSelectionHome(void)
{
    if (menuRxData[0] == 'h')
    {
        bleMenuCb.menuId = BLE_MENU_ID_MAIN;
        bleMenuCb.prevMenuId = BLE_MENU_ID_MAIN;
        bleMenuCb.gapMenuSelected = GAP_MENU_ID_NONE;
        return true;
    }
    return false;
}

//*****************************************************************************
//
// show scan results
//
//*****************************************************************************

#define MAXFRIENDLYNAME 30
static bool showScanResults(void)
{
    appDevInfo_t *devInfo;
    uint8_t num = AppScanGetNumResults();
    char  friendly[MAXFRIENDLYNAME];

    if (num == 0) {
      am_menu_printf("NO devices detected\r\n");
      return (false);
    }
    
    am_menu_printf("--------------------Scan Results--------------------\r\n");
    for (int i = 0; i < num; i++)
    {
        devInfo = AppScanGetResult(i);
        if (devInfo)
        {
            get_friendly_name(devInfo->addr, friendly, MAXFRIENDLYNAME);

            am_menu_printf("%d : %d %02X:%02X:%02X:%02X:%02X:%02X %s\r\n", i, devInfo->addrType,
            devInfo->addr[5], devInfo->addr[4], devInfo->addr[3], devInfo->addr[2], devInfo->addr[1], devInfo->addr[0], friendly);
        }
    }
    am_menu_printf("-----------------------------------------------------\r\n");

    return(true);
}

//*****************************************************************************
//
// handle Gap Selection
//
//*****************************************************************************
static void handleGAPSlection(void)
{
    eGapMenuId id;
    if (bleMenuCb.gapMenuSelected == GAP_MENU_ID_NONE)
    {
        id = (eGapMenuId)(menuRxData[0] - '0');
    }
    else
    {
        id = bleMenuCb.gapMenuSelected;
    }

    switch (id)
    {
        case GAP_MENU_ID_SCAN_START:
            am_menu_printf("scan start\r\n");
            AmdtpcScanStart();
            break;
        case GAP_MENU_ID_SCAN_STOP:
            am_menu_printf("scan stop\r\n");
            AmdtpcScanStop();
            break;
        case GAP_MENU_ID_SCAN_RESULTS:
            showScanResults();
            break;
        case GAP_MENU_ID_CONNECT:
            if (bleMenuCb.gapMenuSelected == GAP_MENU_ID_NONE)
            {
                am_menu_printf("choose an idx from scan results to connect:\r\n");
                if (showScanResults())  bleMenuCb.gapMenuSelected = GAP_MENU_ID_CONNECT;
            }
            else
            {
                uint8_t idx = menuRxData[0] - '0';
                am_menu_printf("selected option %d\n", idx);
                bleMenuCb.gapMenuSelected = GAP_MENU_ID_NONE;
                AmdtpcConnOpen(idx);
            }
            break;

        case GAP_MENU_ID_CLOSE:
            AmdtpcConnClose();
            break;

        case GAP_MENU_ID_TRHOUGHPUT:
            MEASURE_THROUGHPUT = !MEASURE_THROUGHPUT;
            if (MEASURE_THROUGHPUT) am_menu_printf("Throughput enabled\n");
            else am_menu_printf("Throughput disabled\n");
        default:
            break;
    }
}

//*****************************************************************************
//
// Pass keyboard input for the right menu
//
//*****************************************************************************
void handleSelection(void)
{
    if (isSelectionHome())
    {
        BleMenuShowMenu();
        return;
    }

    switch (bleMenuCb.menuId)
    {
        case BLE_MENU_ID_MAIN:
            bleMenuCb.menuId = (eBleMenuId)(menuRxData[0] - '0');
            BleMenuShowMenu();
            break;
        case BLE_MENU_ID_GAP:
            handleGAPSlection();
            break;
        case BLE_MENU_ID_GATT:
            // TODO
            break;
        case BLE_MENU_ID_AMDTP:
            handleAMDTPSlection();
            break;
        default:
            am_menu_printf("handleSelection() unknown input\n");
            break;
    }
}

//*****************************************************************************
//
// display notifications
//
//*****************************************************************************
void handleNotification()
{
  switch (menuRxData[0])
  {
    case NOT_CONN_OPENED:
      am_menu_printf("Connection opened\r\n");
      bleMenuCb.menuId = BLE_MENU_ID_MAIN;
      BleMenuShowMenu();
      break;
      
    case NOT_CONN_CLOSED:
      am_menu_printf("Connection closed\r\n");
      BleMenuShowMenu();
      break;
      
    case NOT_SCAN_STOP:
      am_menu_printf("Scanning finished. ");
      uint8_t num = AppScanGetNumResults();
      if (num > 0) am_menu_printf("%d devices discovered\r\n", num);
      else am_menu_printf("No devices discovered\r\n");
      break;
      
    case NOT_SCAN_START:
      am_menu_printf("Scanning started\r\n");
      break;
    default:
        break;
  }
}

static void
BLEMenuShowMainMenu(void)
{

    am_menu_printf("--------------------BLE main menu--------------------\r\n");
    for (int i = 0; i < BLE_MENU_ID_MAX; i++)
    {
        am_menu_printf("%s\r\n", mainMenuContent[i]);
    }
    am_menu_printf("hint: use 'h' to do main menu\r\n");
    am_menu_printf("-----------------------------------------------------\r\n");
}

static void
BLEMenuShowGAPMenu(void)
{
    am_menu_printf("----------------------GAP menu----------------------\r\n");
    for (int i = 0; i < GAP_MENU_ID_MAX; i++)
    {
        am_menu_printf("%s\r\n", gapMenuContent[i]);
    }
    am_menu_printf("hint: use 'h' to do main menu\r\n");
    am_menu_printf("-----------------------------------------------------\r\n");
}

static void
BLEMenuShowGATTMenu(void)
{
    am_menu_printf("----------------------GATT menu----------------------\r\n");
    for (int i = 0; i < GATT_MENU_ID_MAX; i++)
    {
        am_menu_printf("%s\r\n", gattMenuContent[i]);
    }
    am_menu_printf("hint: use 'h' to do main menu\r\n");
    am_menu_printf("-----------------------------------------------------\r\n");
}

static void
BLEMenuShowAMDTPMenu(void)
{
   am_menu_printf("----------------------AMDTP menu----------------------\r\n");
   for (int i = 0; i < AMDTP_MENU_MAX_ENTRIES; i++)
    {
        if(strlen(amdtpMenuContent[i]) == 0) break;
        am_menu_printf("%s\r\n", amdtpMenuContent[i]);
    }
    am_menu_printf("\nhint: use 'h' to do main menu\r\n");
    am_menu_printf("-----------------------------------------------------\r\n");
}

//*****************************************************************************
//
// select menu to show
//
//*****************************************************************************
void
BleMenuShowMenu(void)
{
    switch (bleMenuCb.menuId)
    {
        case BLE_MENU_ID_MAIN:
            BLEMenuShowMainMenu();
            break;
        case BLE_MENU_ID_GAP:
            BLEMenuShowGAPMenu();
            break;
        case BLE_MENU_ID_GATT:
            BLEMenuShowGATTMenu();
            break;
        case BLE_MENU_ID_AMDTP:
            BLEMenuShowAMDTPMenu();
            break;
        default:
            break;
    }
}
//*****************************************************************************
//
// initialize the menu structure
//
//*****************************************************************************
void
BleMenuInit(void)
{
    bleMenuCb.prevMenuId = BLE_MENU_ID_MAIN;
    bleMenuCb.menuId = BLE_MENU_ID_MAIN;
    bleMenuCb.gapMenuSelected = GAP_MENU_ID_NONE;
    bleMenuCb.gattMenuSelected = GATT_MENU_ID_NONE;
    BleMenuShowMenu();
}
