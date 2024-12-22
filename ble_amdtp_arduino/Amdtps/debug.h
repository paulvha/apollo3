/* 
 * Enable debug messages across amdtps
 *  
 *  
 *  Copyright (c) 2020 rights reserved.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
 *  to any use, copying or further distribution of this software.
 *  
  *  Version 1.0 / February 2020 / paulvha
 */

// uncomment to enable debug messages, comment to disable
// show function flow + sending and receiving data
//#define BLE_Debug 

// uncomment to show sending and receiving data only
//#define BLE_SHOW_DATA 

// enable debug messages from driver (if any defined)??
// requires also change in am_util_debug.h !!
// add #define AM_DEBUG_PRINTF in top (or NOT)
//#define AM_DEBUG_PRINTF

/* ENABLE DATA with DEBUG */
#if defined BLE_Debug
#define BLE_SHOW_DATA 
#endif

/* DEVELOPER DRIVER TESTS (UNDOCUMENTED): 
  For the driver DEVELOPER TEST
  
  AMDTPS_RXONLY: DEVELOPER test for lower level Bleutooth
   Will only count the total bytes or raw data received from a client and display the total result
   No information or acknowledgements etc are ent
  
  AMDTPS_RX2TX :DEVELOPER loop back test
   Will perform a sent back to the client the same raw data is had received.
   Will count the total bytes or raw data received from a client and display the total result
   No acknowledgementsare sent
   
  MEASURE_THROUGHPUT : DEVELOPER
   will show the number of bytes exchanged.
*/

// do read only
//#define AMDTPS_RXONLY 1

// loop back received data
//#define AMDTPS_RX2TX  1

// measure the number of bytes exchanged
//#define MEASURE_THROUGHPUT 1
