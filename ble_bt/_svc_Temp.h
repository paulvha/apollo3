/*************************************************************************************************/
/*!
 *  \file   _svc_Temp.h
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
 *  Based on the original svc_temp.h but removed, added and updated.
 */
/*************************************************************************************************/

#ifndef SVC_TEMP_H
#define SVC_TEMP_H

#define ATT_UUID_ENVIRONMENT_SERVICE            0x181A    /*! environment sensing Service */

#ifdef __cplusplus
extern "C" {
#endif


/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

#define TEMP_HANDLE_START  0x70
#define TEMP_HANDLE_END   (TEMP_HANDLE_END_PLUS_ONE - 1)

/**************************************************************************************************
 Handles
**************************************************************************************************/

/* Temperature service handles. */
enum
{
  TEMP_HANDLE_SVC = TEMP_HANDLE_START,

  TEMP_HANDLE_DATA_CHRC,
  TEMP_HANDLE_DATAC,
  TEMP_HANDLE_DATA_CHRF,
  TEMP_HANDLE_DATAF,
  TEMP_HANDLE_DATA_CLIENT_CHR_CONFIG,
  TEMP_HANDLE_DATA_CHR_USR_DESCR,

  TEMP_HANDLE_END_PLUS_ONE
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

void SvcTempAddGroup(void);
void SvcTempRemoveGroup(void);
void SvcTempCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);


#ifdef __cplusplus
};
#endif

#endif /* SVC_TEMP_H */
