/*************************************************************************************************
 *
 *  \file   _svc_Batt.h
 *
 *  \brief  Example Battery service implementation.
 *
 *          $Date: 2017-02-08 14:15:55 -0600 (Wed, 08 Feb 2017) $
 *          $Revision: 11118 $
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
 *  Based on the original svc_batt.h but removed, added and updated.
 *
 *************************************************************************************************/

#ifndef SVC_BATT_H
#define SVC_BATT_H

#ifdef __cplusplus
extern "C" {
#endif

/*! battery load generated with https://www.uuidgenerator.net/version4 
* fc8b661f-86d5-4e33-97a4-b3fef6772838 */
#define  BATT_UUID_CHR_CONFIG_BASE      0x38, 0x28, 0x77, 0xf6, 0xfe, 0xb3, \
                                        0xa4, 0x97, 0x33, 0x4e, 0xd5, 0x86, \
                                        0x1f, 0x66, 0x8b, 0xfc
/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

/* Battery Service */
#define BATT_START_HDL                    0x60
#define BATT_END_HDL                      (BATT_MAX_HDL - 1)

/**************************************************************************************************
 Handles
**************************************************************************************************/

/* Battery Service Handles */
enum
{
  BATT_SVC_HDL = BATT_START_HDL,        /* Battery service declaration */
  BATT_LVL_CH_HDL,                      /* Battery level characteristic */
  BATT_LVL_HDL,                         /* Battery level */
  BATT_HANDLE_CONFIG_CHR,               /* battery load characteristic*/
  BATT_HANDLE_CONFIG,
  BATT_HANDLE_CONFIG_CHR_USR_DESCR,
  BATT_LVL_CH_CCC_HDL,                  /* Battery level CCCD */
  BATT_MAX_HDL
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void SvcBattAddGroup(void);
void SvcBattRemoveGroup(void);
void SvcBattCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);


#ifdef __cplusplus
};
#endif

#endif /* SVC_BATT_H */
