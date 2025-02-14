/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  ********************************************************************
  This is a special for running ArduinoBLE on ExactLE (and thus 1.2.1)
  based on HCICordioTransport.cpp.  paulvha / February 2021

  Updated and tested with V1.2.3    paulvha / January 2023
  added changes to enable deepsleep

  Sometimes you get :
  Error while detecting libraries included by /home/paul/Arduino/libraries/ArduinoBLE/src/utility/HCIExactleTransport.cpp
  This is a bug in Arduino IDE
  * https://github.com/MarlinFirmware/Marlin/issues/19720
  * https://forum.arduino.cc/index.php?topic=591592.0
*/

// defined for Apollo3 1.2.3. on later levels use HCICordioTransport
#if defined (ARDUINO_ARCH_APOLLO3) && ! defined (ARDUINO_ARCH_MBED)

// enable debug messages from Exactle (set to 1)
#define PRINT_DEBUG_TRACE 0

#include <Arduino.h>
#include "HCIExactleTransport.h"
#include "RingBuffer.h"

RingBufferN<256> _rxBuf;

#ifdef __cplusplus
extern "C"
{
#endif

#include "wsf_types.h"
#include "wsf_buf.h"
#include "wsf_msg.h"
#include "hci_drv_apollo.h"
#include "hci_drv_apollo3.h"
#include "hci_drv.h"
#include "wsf_timer.h"
#include "wsf_trace.h"

#ifdef __cplusplus
}
#endif

#ifndef CORDIO_ZERO_COPY_HCI
ERROR: EXACTLE IMPLEMENTATION ONLY WORKS WITH ZERO COPY
#endif

// Parts of this file are based on: https://github.com/ARMmbed/mbed-os-cordio-hci-passthrough/pull/2
// With permission from the Arm Mbed team to re-license

/* avoid many small allocs (and WSF doesn't have smaller buffers) */
#define MIN_WSF_ALLOC (16)

HCIExactleTransportClass::HCIExactleTransportClass() :
  _begun(false) // not begun yet
{
}

HCIExactleTransportClass::~HCIExactleTransportClass()
{
}

int HCIExactleTransportClass::begin()
{
  _rxBuf.clear();

#if PRINT_DEBUG_TRACE
  WsfTraceRegisterHandler(traceCback);    // will set in wsf_trace.c generic
  WsfTraceEnable(true);                   // will set in wsf_trace.c generic
#endif

  //
  // Boot the radio. (act as cold boot to let 32Khz stabilize)
  //
  HciDrvRadioBoot(1);

  //
  // Initialize the main ExactLE stack.
  //
  exactle_init();

  //
  // set on data received is called
  //
  HciDrvset_data_received_handler(HCIExactleTransportClass::onDataReceived);

  _begun = true;                    // all is ready to go

  return 1;
}

void HCIExactleTransportClass::end()
{
  HciDrvRadioShutdown();

  _begun = false;
}

// special paulvha : set TX power Feb 2025
bool HCIExactleTransportClass::setTXPower(uint8_t TXpower) 
{
  // defined in hci_drv_apollo3.h  / ExactleP as txPowerLevel_t valid values
  if (TXpower != TX_POWER_LEVEL_MINUS_10P0_dBm &&
  TXpower != TX_POWER_LEVEL_0P0_dBm && TXpower != TX_POWER_LEVEL_PLUS_3P0_dBm )
    return (false);
  
  // perform the Apollo3 setting (in hci_drv_apollo3.c / ExactleP
  return(HciVsA3_SetRfPowerLevelEx((txPowerLevel_t)TXpower));
}


// called from poll() in case it needs to wait for data
// for a certain time
void HCIExactleTransportClass::wait(unsigned long timeout)
{
  if (available()) return;

  for (unsigned long start = millis(); (millis() - start) < timeout;) {
    if (available()) break;
    delay(10);
  }
}

// check for data in the RXbuffer
// if nothing trigger the timers and driver
int HCIExactleTransportClass::available()
{
  if (_rxBuf.available())  return _rxBuf.available();

  // trigger BLE driver
  poll();

  return _rxBuf.available();
}

// peek for first data in RXbuffer
int HCIExactleTransportClass::peek()
{
  return _rxBuf.peek();
}

// read from RXbuffer data
int HCIExactleTransportClass::read()
{
  return _rxBuf.read_char();
}

// write data to BLE
size_t HCIExactleTransportClass::write(const uint8_t* data, size_t length)
{
  if (!_begun) {
    return 0;
  }

  uint8_t packetLength = length - 1;
  uint8_t packetType   = data[0];

  uint8_t* packet = (uint8_t*) WsfMsgAlloc(max(packetLength, MIN_WSF_ALLOC));

  if (packet == NULL) {
     Serial.println("HCIExactleTransport::write");
     Serial.println("ERROR: Could not obtain memory");
     return(0);
  }

  // skip packet type
  memcpy(packet, &data[1], packetLength);

  // 'concept' is taken from AP3CordioCITRansportDriver.cpp 2.0.3
  // Temporary workaround, random address not working, suppress it.
  if (data[0] == 0x06 && data[1] == 0x20)
  {
      packet[7] = 0;
  }

  uint16_t retLen = hciDrvWrite(packetType, (uint16_t) packetLength, packet);

  WsfMsgFree(packet);

  return retLen;
}

// store received data in the _rxBuf, taken from HCICordioTransport.cpp
// _rxbuf is a ring buffer 256
void HCIExactleTransportClass::handleRxData(uint8_t* data, uint8_t len)
{
  // patch #96 not loosing data
  while (_rxBuf.availableForStore() < len) {
    // Wait for free space on RingBuffer
    yield();
  }

  for (int i = 0; i < len; i++) {
    _rxBuf.store_char(data[i]);
  }
}

HCIExactleTransportClass HCIExactleTransport;
HCITransportInterface& HCITransport = HCIExactleTransport;

// call back from lower stacklevel calls function above
void HCIExactleTransportClass::onDataReceived(uint8_t* data, uint8_t len)
{
  HCIExactleTransport.handleRxData(data, len);
}

//*****************************************************************************
//
// This routine will trigger WSF event handler
// make sure this called often as part of your program loop
//
//*****************************************************************************
bool
HCIExactleTransportClass::poll()
{
    wsfOsDispatcher();
    return(true);
}

//*****************************************************************************
//
// WSF buffer pools.
//
//*****************************************************************************
#define WSF_BUF_POOLS               4

// Important note: the size of g_pui32BufMem should includes both overhead of internal
// buffer management structure, wsfBufPool_t (up to 16 bytes for each pool), and pool
// description (e.g. g_psPoolDescriptors below).

// Memory for the buffer pool
static uint32_t g_pui32BufMem[(WSF_BUF_POOLS*16
         + 16*8 + 32*4 + 64*6 + 280*8) / sizeof(uint32_t)];

// Default pool descriptor.
static wsfBufPoolDesc_t g_psPoolDescriptors[WSF_BUF_POOLS] =
{
    {  16,  8 },
    {  32,  4 },
    {  64,  6 },
    { 280,  8 }
};

//*****************************************************************************
//
// Initialization for the ExactLE stack.
//
//*****************************************************************************
void
exactle_init(void) {
    /*! In case BLE is used in an environment where the Apollo3 board is set to deepsleep, every time after wake-up
     * a next handlernumber is requested. The maximum is 9 (defined in wsf_os.c).
     * Once you run out of the 9.. BLE will not startup again. There is no call to free the handler-number in WSF.
     * Hence we set the handlerid to 0xAA at start, once it obtained a handlerid, that ID will continue to be used.*/
    static wsfHandlerId_t handlerId = 0xAA;

    //
    // Initialize a buffer pool for WSF dynamic memory needs.
    //
    WsfBufInit(sizeof(g_pui32BufMem), (uint8_t*)g_pui32BufMem, WSF_BUF_POOLS, g_psPoolDescriptors);

    //
    // Set up functions for the ExactLE DRIVER
    // ONLY the Apollo3 driver is used.. nothing else from Exactle...NO HCI, NO ATT, NO LSCAP, NO GAP, NO security
    // ALL that is handled by ArduinoBLE.
    //
    if (handlerId == 0xAA) {
      handlerId = WsfOsSetNextHandler(HciDrvHandler);  // HciDrvHandler() is in hci_drv_apollo3.c
    }
    HciDrvHandlerInit(handlerId);
}

#if PRINT_DEBUG_TRACE
/***********************************************************************************************
 *!
 * call back for debug the Bluetooth stack
 * define PRINT_DEBUG_TRACE in top
 * Requires defines.extra=-DWSF_TRACE_ENABLED in platform.txt (enabled by default)
 * enables for BOTH Generic WSF and ambiq WSF tracking
 *
 * For WSF tracking the message setting needs to be done in thirdparty/exactle/sw/wsf/ambiq/wsf_trace.h
 * and sometimes in thirdparty/exactle/sw/wsf/generic/wsf_trace.h. The compiler makes a choice..
 *
 ***********************************************************************************************/

#define DEBUG_UART_BUF_LEN 256
char debug_buffer [DEBUG_UART_BUF_LEN];

void traceCback(const uint8_t *pBuf, uint32_t len)
{
    debug_printf((char *) "traceCback: ");

    for (uint32_t i = 0; i < len; i++)
    {
        if (pBuf[i] > 0x1f) debug_printf((char *) "%c", pBuf[i]);
        else debug_printf((char *) ".");
    }
    debug_printf((char *) "\r\n");
}

void debug_printf(char* fmt, ...){

    va_list args;
    va_start (args, fmt);

    int c = vsnprintf(debug_buffer, DEBUG_UART_BUF_LEN, (const char*)fmt, args);

    // add CR at end as some program (like Putty seem to need that)
    if (c >0 && c < DEBUG_UART_BUF_LEN - 1)
    {
        debug_buffer[c++] = '\r';
        debug_buffer[c] = 0x0;
    }

    va_end (args);

    Serial.print(debug_buffer);
}
#else
void debug_printf(char* fmt, ...){}   // do nothing

#endif // PRINT_DEBUG_TRACE

//*****************************************************************************
//
// Interrupt handler for the CTIMERs
//
//*****************************************************************************
extern "C" void am_ctimer_isr(void){
    uint32_t ui32Status;

    //
    // Check and clear any active CTIMER interrupts.
    //
    ui32Status = am_hal_ctimer_int_status_get(true);
    am_hal_ctimer_int_clear(ui32Status);

    //
    // Run handlers for the various possible timer events.
    //
    am_hal_ctimer_int_service(ui32Status);
}

//*****************************************************************************
//
// Interrupt handler for BLE
//
//*****************************************************************************
extern "C" void am_ble_isr(void){
    HciDrvIntService();                 // in hci_drv_apollo3.c
}

#endif // defined(ARDUINO_ARCH_APOLLO3)
