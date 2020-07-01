//*****************************************************************************
//// SPECIAL_BUTTON ADDED BUTTON functions 
//  version 1.0 / paulvha / June 2020
//
//! @file svc_buttons.c
//!
//! @brief Buttons implementation
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2019, Ambiq Micro
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
// This is part of revision v2.2.0-7-g63f7c2ba1 of the AmbiqSuite Development Package.
//
//*****************************************************************************
#include "wsf_types.h"
#include "att_api.h"
#include "wsf_trace.h"
#include "bstream.h"
#include "svc_ch.h"
#include "svc_buttons.h"
#include "svc_cfg.h"

/**************************************************************************************************
 Static Variables
**************************************************************************************************/
/* UUIDs */
static const uint8_t svcRxUuid[] = {ATT_UUID_BUTTONS_RX};

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/* Button service declaration */
static const uint8_t buttonSvc[] = {ATT_UUID_BUTTONS_SERVICE};
static const uint16_t buttonLenSvc = sizeof(buttonSvc);

/* Button RX characteristic */
static const uint8_t buttonRxCh[] = {ATT_PROP_WRITE_NO_RSP, UINT16_TO_BYTES(BUTTONS_RX_HDL), ATT_UUID_BUTTONS_RX};
static const uint16_t buttonLenRxCh = sizeof(buttonRxCh);

/* Button RX data */
/* Note these are dummy values */
static const uint8_t buttonRx[] = {0};
static const uint16_t buttonLenRx = sizeof(buttonRx);

/* Proprietary data client characteristic configuration */
static uint8_t buttonRxChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t buttonLenRxChCcc = sizeof(buttonRxChCcc);


/* Attribute list for Button group */
static const attsAttr_t buttonList[] =
{
  {// 0x8000 : 0000183B-08C2-11E1-9073-0E8AC72E1011
    attPrimSvcUuid,
    (uint8_t *) buttonSvc,
    (uint16_t *) &buttonLenSvc,
    sizeof(buttonSvc),
    0,
    ATTS_PERMIT_READ
  },
  { //0x0801, uuid = 0000183B-08C2-11E1-9073-0E8AC72E0011
    attChUuid,
    (uint8_t *) buttonRxCh,
    (uint16_t *) &buttonLenRxCh,
    sizeof(buttonRxCh),
    0,
    ATTS_PERMIT_READ
  },
  { //0x0802, not user
    svcRxUuid,
    (uint8_t *) buttonRx,
    (uint16_t *) &buttonLenRx,
    ATT_VALUE_MAX_LEN,
    (ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    ATTS_PERMIT_WRITE
  },
  { //0x0805, uuid = 0000
    attCliChCfgUuid,
    (uint8_t *) buttonRxChCcc,
    (uint16_t *) &buttonLenRxChCcc,
    sizeof(buttonRxChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  }
};

/* Button group structure */
static attsGroup_t svcbuttonGroup =
{
  NULL,                         // index (used internal)
  (attsAttr_t *) buttonList,     // pointer to the entry poin list
  NULL,                         // read call back
  NULL,                         // write call back
  BUTTONS_START_HDL,
  BUTTONS_END_HDL
};

/*************************************************************************************************/
/*!
 *  \fn     SvcButtonsAddGroup
 *
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcButtonsAddGroup(void)
{
  AttsAddGroup(&svcbuttonGroup);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcbuttonsRemoveGroup
 *
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcbuttonsRemoveGroup(void)
{
  AttsRemoveGroup(BUTTONS_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcButtonsCbackRegister
 *
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcButtonsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcbuttonGroup.readCback = readCback;
  svcbuttonGroup.writeCback = writeCback;
}
