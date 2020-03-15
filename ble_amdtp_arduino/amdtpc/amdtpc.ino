/*
  BLE <-> Arduino client server comm.  CLIENT VERSION derived from ble_freertos_amdtpc SDK app.

  This example demonstrates basic BLE server (peripheral) functionality for the Apollo3 boards.

  How to use this example:

  Make sure to load on another Apollo3 board the BLE_amdtps sketch.
  
  
  Start the server
  Start the client
  Select GAPP menu  (1 <enter>)
  Select Start Scan (1 <enter>)
  Wait for finish
  Select Create connection  (4 < enter>)
  select Artemis AMDTP BLE ( 0 or 1 or 2 ... <enter>)
  wait for open 
  when opened return to main menu ( h < enter>)
  Select amdtp menu ( 3 <enter>)
  Now select the server command that you want to execute

  When finished
  select main menu ( h <enter>)
  select gapp menu ( 1 <enter>)
  select close     ( 5 <enter>)
 
*/
/********************************************************************************************************************
 *******************************************************************************************************************/
#include "Arduino.h"
#include "BLE_example.h"
#include "ble_menu.h"
//*****************************************************************************
//
// Required built-ins.
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

//*****************************************************************************
//
// Standard AmbiqSuite includes.
//
//*****************************************************************************
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#if (defined BLE_MENU) || (defined AM_DEBUG_PRINTF)

// buffer for printf
static char g_prfbuf[AM_MENU_BUFSIZE];

uint32_t
am_menu_printf(const char *pcFmt, ...)
{
    uint32_t ui32NumChars;

    //
    // Convert to the desired string.
    //
    va_list pArgs;
    va_start(pArgs, pcFmt);
    ui32NumChars = am_util_stdio_vsprintf(g_prfbuf, pcFmt, pArgs);
    va_end(pArgs);

    Serial.print(g_prfbuf);

    return ui32NumChars;
}
#else

  uint32_t
  am_menu_printf(const char *pcFmt, ...)
  {
      return 0;
  }

#endif //(defined BLE_MENU) || (defined AM_DEBUG_PRINTF)


void setup() {

  Serial.begin(115200);

  Serial.println("Arduino AMDTP Example");

#ifdef AM_DEBUG_PRINTF
  //
  // Enable printing to the console. *defined in bleu_menu.h
  //
  enable_print_interface();

  am_util_debug_printf("\n AMDTP Example Print Debug\n");
#endif
  /********************************************************************************************************************
                   Boot the radio
                    SDK/third_party/exactle/sw/hci/apollo3/hci_drv_apollo3.c
                    = huge program to handle the ble radio stuff in this file
   *******************************************************************************************************************/
  HciDrvRadioBoot(0);

#ifdef AM_DEBUG_PRINTF
  am_util_debug_printf("Calling exactle_stack_init...\n");
#endif

  /************************************************************************************************
        Initialize the main ExactLE stack: BLE_example_funcs.cpp
        - One time timer
        - timer for handler
        - dynamic memory buffer
        - security
        - HCI host conroller interface
        - DM device manager software
        - L2CAP data transfer management
        - ATT - Low Energy handlers
        - SMP - Low Energy security
        - APP - application handlers..global settings..etc
        - NUS - nordic location services

   ************************************************************************************************/
  exactle_stack_init();

#ifdef AM_DEBUG_PRINTF
  am_util_debug_printf("Finished exactle_stack_init...\n");
#endif

  /*************************************************************************************************
      Start the "Amdtp" (AmbiqMicro Data Transfer Protocol) profile. Function in amdtp_main.c

       Register for stack callbacks
       - Register callback with DM for scan and advertising events with security
       - Register callback with Connection Manager with client id
       - Register callback with ATT low energy handlers
       - Register callback with ATT low enerty connection handlers
       - Register callback with ATT CCC = client charachteristic configuration array
       - Register for app framework discovery callbacks
       - Initialize attribute server database
       - Reset the device

   ************************************************************************************************/
#ifdef AM_DEBUG_PRINTF
  am_util_debug_printf("calling AmdtpcStart.....\n");
#endif

  AmdtpcStart();
}

void loop() {

      static uint16_t cnt = 0;

      // handle any keyboard input
      if(Serial.available()) {
        while (Serial.available()) add_menu_input(Serial.read());
      }

      //
      // Calculate the elapsed time from our free-running timer, and update
      // the software timers in the WSF scheduler.
      //
      update_scheduler_timers();
      wsfOsDispatcher();

      //
      // Enable an interrupt to wake us up next time we have a scheduled event.
      //
      set_next_wakeup();

      am_hal_interrupt_master_disable();

      //
      // Check to see if the WSF routines are ready to go to sleep.
      //
      if ( wsfOsReadyToSleep() )
      {
          am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
      }

      am_hal_interrupt_master_enable();
}
