/*************************************************************************************************/
/*!
 *  \file   crc32.h
 *
 *  \brief  CRC-32 utilities.
 *
 *          $Date: 2016-12-28 16:12:14 -0600 (Wed, 28 Dec 2016) $
 *          $Revision: 10805 $
 *
 *  Copyright (c) 2010-2017 ARM Ltd., all rights reserved.
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
 */////////////////////////////////////////////////////////////////////////////////////////////
 //  This CRC MIGHT NOT BE NEEEDED. IT IS INCLUDED IN MBED AND NEW RELEASE APOLLO3 LIBRARY 2.0.3
 //  BUT IF NEEDED :
 //
 //  UNCOMMENT IN amdtps_protocol.c line 55
 //  //#include "crc32.h"
 //
 //  Reename in CRC32.c, on line 120
 //  CalcCrc32_org to CalcCrc32

/*************************************************************************************************/
#ifndef CRC32_H
#define CRC32_H

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************************************/
/*!
 *  \fn     CalcCrc32
 *
 *  \brief  Calculate the CRC-32 of the given buffer.
 *
 *  \param  crcInit  Initial value of the CRC.
 *  \param  len      Length of the buffer.
 *  \param  pBuf     Buffer to compute the CRC.
 *
 *  \return None.
 *
 *  This routine was originally generated with crcmod.py using the following parameters:
 *    - polynomial 0x104C11DB7
 *    - bit reverse algorithm
 */////////////////////////////////////////////////////////////////////////////////////////////
 //  This CRC MIGHT NOT BE NEEEDED. IT IS INCLUDED IN NEW MBED AND NEW RELEASE APOLLO3 LIBRARY 2.0.3
 //  BUT IF NEEDED :
 //
 //  UNCOMMENT IN amdtp_common.c line 55
 //  //#include "crc32.h"
 //
 //  Rename in crc32.c, on line 128
 //  CalcCrc32_org to CalcCrc32
/*************************************************************************************************/
uint32_t CalcCrc32(uint32_t crcInit, uint32_t len, uint8_t *pBuf);

#ifdef __cplusplus
};
#endif

#endif /* CRC32_H */
