/*
 * in this tab you will find defines that are commonly
 * used in the other tabs.
 * 
 * version 1.0/ November 2022
 * 
 * paulvha
 */

/* 
 *  BLE_DEBUG will show the routines and debug information
 * BLE_SHOW_DATA will show the data being received and sent
 *
 * uncomment / remove '//' to enable
 */
//#define BLE_Debug 1
//#define BLE_SHOW_DATA 1

/* ENABLE DATA with DEBUG */
#if defined BLE_Debug
#define BLE_SHOW_DATA 1
#endif

// define the serial port
#define SERIAL_PORT Serial
