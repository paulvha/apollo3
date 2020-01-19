/*************************************************************************************************/
/*!
 *  \file   _svc_Batt.c
 *
 *  \brief  Example Battery service implementation.
 *
 *          $Date: 2017-02-07 19:05:49 -0600 (Tue, 07 Feb 2017) $
 *          $Revision: 11110 $
 *
 *  Copyright (c) 2011-2017 ARM Ltd., all rights reserved.
 *  ARM Ltd. confidential and proprietary.
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
 *  Based on the original svc_batt.c but removed, added and updated.
 *  
 *  version 1.0.1 / January 2020 / paulvha
 *  - change update of values with AttsSetAttr()
 *  - set the battery load resistor UUID as 128 bits
 */
/*************************************************************************************************/

#include "wsf_types.h"
#include "att_api.h"
#include "bstream.h"
#include "_svc_Batt.h"
#include "svc_cfg.h"

#include "ble_debug.h" 

#ifdef BLE_Debug
extern void debug_print(const char* f, const char* F, uint16_t L);
extern void debug_printf(char* fmt, ...);
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef BATT_SEC_PERMIT_READ
#define BATT_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef BATT_SEC_PERMIT_WRITE
#define BATT_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif
                                      
/*************************************************************************************************
 Battery Service group
**************************************************************************************************/

/*!
 * Battery service
 */

/* Battery service declaration */
static const uint8_t battValSvc[] = {UINT16_TO_BYTES(ATT_UUID_BATTERY_SERVICE)};
static const uint16_t battLenSvc = sizeof(battValSvc);

/* Battery level characteristic */
static const uint8_t battValLvlCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(BATT_LVL_HDL), UINT16_TO_BYTES(ATT_UUID_BATTERY_LEVEL)};
static const uint16_t battLenLvlCh = sizeof(battValLvlCh);

/* Battery level */
static uint8_t battValLvl[] = {0};
static const uint16_t battLenLvl = sizeof(battValLvl);

/* Battery level client characteristic configuration */
static uint8_t battValLvlChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t battLenLvlChCcc = sizeof(battValLvlChCcc);

/* Battery load resistor characteristic. */
static const uint8_t  batValConfigChr[] = {ATT_PROP_READ | ATT_PROP_WRITE,
                                            UINT16_TO_BYTES(BATT_HANDLE_CONFIG),
                                            BATT_UUID_CHR_CONFIG_BASE};
static const uint16_t batLenConfigChr   = sizeof(batValConfigChr);

/* Battery load resistor config. */
static const uint8_t  batUuidConfig[] = {BATT_UUID_CHR_CONFIG_BASE};
static       uint8_t  batValConfig[]  = {0x00};
static const uint16_t batLenConfig    = sizeof(batValConfig);

/* Battery load characteristic user description. */
static const uint8_t  batValConfigChrUsrDescr[] = "battery load : 1=enable 0=remove";
static const uint16_t batLenConfigChrUsrDescr   = sizeof(batValConfigChrUsrDescr) - 1u;


/* Attribute list for group */
static const attsAttr_t battList[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid,           // 0x2800 MUST be first
    (uint8_t *) battValSvc,   // 0x180F describes the service
    (uint16_t *) &battLenSvc,
    sizeof(battValSvc),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic declaration */
  {
    attChUuid,                // 0x2803 declare what kind of value is in the characteristic and which handle to get the value
    (uint8_t *) battValLvlCh, // handle for 0x2A19
    (uint16_t *) &battLenLvlCh,
    sizeof(battValLvlCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value */
  {
    attBlChUuid,            // 0x2A19 value
    battValLvl,
    (uint16_t *) &battLenLvl,
    sizeof(battValLvl),
    ATTS_SET_READ_CBACK,
    BATT_SEC_PERMIT_READ
  },

 /* Characteristic declaration.: adding / removing Battery load */
  {
    attChUuid,            // 0x2803 declare the battery load characteristic
    (uint8_t *) batValConfigChr,
    (uint16_t *) &batLenConfigChr,
    sizeof(batValConfigChr),
    0,
    ATTS_PERMIT_READ
  },
  /* Characteristic value.  */
  {
    batUuidConfig,        // None Standaard UUID defined in scv_batt.h
    (uint8_t *) batValConfig,
    (uint16_t *) &batLenConfig,
    sizeof(batValConfig),
    ATTS_SET_WRITE_CBACK | ATTS_SET_READ_CBACK |ATTS_SET_UUID_128,  // 128 bytes
    ATTS_PERMIT_READ | ATTS_PERMIT_WRITE
  },
  /* Characteristic user description. */
  {
    attChUserDescUuid,   // 0x2901  user description human readable
    (uint8_t *) batValConfigChrUsrDescr,
    (uint16_t *) &batLenConfigChrUsrDescr,
    sizeof(batValConfigChrUsrDescr),
    0,
    ATTS_PERMIT_READ
  },
  
  /* Characteristic CCC descriptor */ 
  {
    attCliChCfgUuid,    // 0x2902
    battValLvlChCcc,
    (uint16_t *) &battLenLvlChCcc,
    sizeof(battValLvlChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  }
};

/* Battery group structure */
static attsGroup_t svcBattGroup =
{
  NULL,
  (attsAttr_t *) battList,
  NULL,
  NULL,
  BATT_START_HDL,
  BATT_END_HDL
};

/*************************************************************************************************/
/*!
 *  \fn     SvcBattAddGroup
 *
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcBattAddGroup(void)
{
  AttsAddGroup(&svcBattGroup);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcBattRemoveGroup
 *
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcBattRemoveGroup(void)
{
  AttsRemoveGroup(BATT_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcBattCbackRegister
 *
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcBattCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcBattGroup.readCback = readCback;
  svcBattGroup.writeCback = writeCback;
}
