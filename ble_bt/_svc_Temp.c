/*************************************************************************************************/
/*!
 *  \file   _svc_Temp.c
 *
 *  \brief  Example temperature sensor service implementation.
*
 *          $Date: 2016-12-28 16:12:14 -0600 (Wed, 28 Dec 2016) $
 *          $Revision: 10805 $
 *
 *  Copyright (c) 2015-2017 ARM Ltd., all rights reserved.
 *  ARM confidential and proprietary.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
 *  to any use, copying or further distribution of this software.
 *  
 *  Version 1.0 / January 2020 / paulvha
 *  Based on the original svc_temp.c but removed, added and updated.
 *  
 *  version 1.0.1 / January 2020 / paulvha
 *  change update of values with AttsSetAttr()
 */
/*************************************************************************************************/

#include <string.h>

#include "wsf_types.h"
#include "att_api.h"
#include "bstream.h"
#include "svc_cfg.h"
#include "_svc_Temp.h"

#include "ble_debug.h" 

#ifdef BLE_Debug
extern void debug_print(const char* f, const char* F, uint16_t L);
extern void debug_printf(char* fmt, ...);
#endif

/**************************************************************************************************
 Macros
**************************************************************************************************/

// https://www.bluetooth.com/specifications/gatt/characteristics/
// ???  0x2A6E // Mandatory : Temperature  : Unit is in degrees Celsius with a resolution of 0.01 degrees Celsius
// ATT_UUID_TEMP_C 0x2A1F // Mandatory :     Temperature Celsius
// ATT_UUID_TEMP_F 0x2A20 // Mandatory :     Temperature Fahrenheit
// ATT_UUID_TEMP_MEAS 0x2A1C // Optional :   Temperature Measurement depending on flag,multiple information can be valid
// ATT_UUID_TEMP_TYPE 0x2A1D // optional :   Temperature Type : place of sensor
/**************************************************************************************************
 Service variables
**************************************************************************************************/

/* Declaration environmental service. */
static const uint8_t  tempValSvc[] = {UINT16_TO_BYTES(ATT_UUID_ENVIRONMENT_SERVICE)};
static const uint16_t tempLenSvc   = sizeof(tempValSvc);

/* Temperature data characteristic. Celsius */
static const uint8_t  tempValDataChrC[] = {ATT_PROP_READ,
                                          UINT16_TO_BYTES(TEMP_HANDLE_DATAC),
                                          UINT16_TO_BYTES(ATT_UUID_TEMP_C)};
static const uint16_t tempLenDataChrC   = sizeof(tempValDataChrC);

/* Temperature data. Celsius */
static const uint8_t  tempUuidDataC[] = {UINT16_TO_BYTES(ATT_UUID_TEMP_C)};
static       uint8_t  tempValDataC[]  = {0x00, 0x00};
static const uint16_t tempLenDataC    = sizeof(tempValDataC);

/* Temperature data characteristic. Fahrenheit*/
static const uint8_t  tempValDataChrF[] = {ATT_PROP_READ,
                                          UINT16_TO_BYTES(TEMP_HANDLE_DATAF),
                                          UINT16_TO_BYTES(ATT_UUID_TEMP_F)};
static const uint16_t tempLenDataChrF   = sizeof(tempValDataChrF);

/* Temperature data. Fahrenheit */
static const uint8_t  tempUuidDataF[] = {UINT16_TO_BYTES(ATT_UUID_TEMP_F)};
static       uint8_t  tempValDataF[]  = {0x00, 0x00};
static const uint16_t tempLenDataF    = sizeof(tempValDataF);

/* Temperature data client characteristic configuration. */
static       uint8_t  tempValDataClientChrConfig[] = {0x00, 0x00};
static const uint16_t tempLenDataClientChrConfig   = sizeof(tempValDataClientChrConfig);

/* Temperature data characteristic user description. */
static const uint8_t  tempValDataChrUsrDescr[] = "Artimis Temp Data";
static const uint16_t tempLenDataChrUsrDescr   = sizeof(tempValDataChrUsrDescr) - 1u;

/* Attribute list for temp group. */
static const attsAttr_t tempList[] =
{
  /* Service declaration. */
  {
    attPrimSvcUuid,           //0x2801 define the service
    (uint8_t *) tempValSvc,
    (uint16_t *) &tempLenSvc,
    sizeof(tempValSvc),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic declaration. Celsius */
  {
    attChUuid,              //0x2803 Define characteristic and handle for value
    (uint8_t *) tempValDataChrC,
    (uint16_t *) &tempLenDataChrC,
    sizeof(tempValDataChrC),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value. */
  {
    tempUuidDataC,          //0x2A1F
    (uint8_t *) tempValDataC,
    (uint16_t *) &tempLenDataC,
    sizeof(tempValDataC),
    ATTS_SET_READ_CBACK,
    ATTS_PERMIT_READ
  },
  /* Characteristic declaration. Fahrenheit*/
  {
    attChUuid,            //0x2803 define characteristic and handle for value
    (uint8_t *) tempValDataChrF,
    (uint16_t *) &tempLenDataChrF,
    sizeof(tempValDataChrF),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value.Fahrenheit */
  {
    tempUuidDataF,      //0x2A20
    (uint8_t*) tempValDataF,
    (uint16_t *) &tempLenDataF,
    sizeof(tempValDataF),
    ATTS_SET_READ_CBACK,
    ATTS_PERMIT_READ
  },
    /* Characteristic user description. */
  {
    attChUserDescUuid, // 0x2901 human readable description
    (uint8_t *) tempValDataChrUsrDescr,
    (uint16_t *) &tempLenDataChrUsrDescr,
    sizeof(tempValDataChrUsrDescr) - 1,
    0,
    ATTS_PERMIT_READ
  },  
  /* Client characteristic configuration. */
  {
    attCliChCfgUuid,    // 0x2902
    (uint8_t *) tempValDataClientChrConfig,
    (uint16_t *) &tempLenDataClientChrConfig,
    sizeof(tempValDataClientChrConfig),
    ATTS_SET_CCC,
    ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
  }
};

/* temp group structure. */
static attsGroup_t tempGroup =
{
  NULL,
  (attsAttr_t *) tempList,
  NULL,
  NULL,
  TEMP_HANDLE_START,
  TEMP_HANDLE_END
};

/*************************************************************************************************/
/*!
 *  \fn     SvcTempAddGroup
 *
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcTempAddGroup(void)
{
  AttsAddGroup(&tempGroup);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcTempRemoveGroup
 *
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcTempRemoveGroup(void)
{
  AttsRemoveGroup(TEMP_HANDLE_START);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcTempCbackRegister
 *
 *  \brief  Register callbacks for the service.
 *
 *  \param  writeCback  Write callback function.
 *  \param  readCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcTempCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  tempGroup.writeCback = writeCback;
  tempGroup.readCback = readCback;
}
