//*****************************************************************************
//// SPECIAL_BUTTON ADDED BUTTON functions 
//  version 1.0 / paulvha / June 2020
//
//! @file svc_buttons.h
//!
//! @brief Buttons
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
#ifndef SVC_BUTTONS_H
#define SVC_BUTtONS_H

//
// Put additional includes here if necessary.
//

#ifdef __cplusplus
extern "C"
{
#endif
//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************

/*! Base UUID:  0000183B-08C2-11E1-9073-0E8AC72EXXXX */
#define ATT_UUID_BUTTONS_BASE             0x2E, 0xC7, 0x8A, 0x0E, 0x73, 0x90, \
                                        0xE1, 0x11, 0xC2, 0x08, 0x3B, 0x18, 0x00, 0x00

/*! Macro for building Buttons UUIDs */
#define ATT_UUID_BUTTONS_BUILD(part)      UINT16_TO_BYTES(part), ATT_UUID_BUTTONS_BASE

/*! Partial Buttons service UUIDs */
#define ATT_UUID_BUTTONS_SERVICE_PART     0x1011

/*! Partial Buttons rx characteristic UUIDs */
#define ATT_UUID_BUTTONS_RX_PART          0x0011

/* BUTTONS services */
#define ATT_UUID_BUTTONS_SERVICE          ATT_UUID_BUTTONS_BUILD(ATT_UUID_BUTTONS_SERVICE_PART)

/* BUTTONs characteristics */
#define ATT_UUID_BUTTONS_RX               ATT_UUID_BUTTONS_BUILD(ATT_UUID_BUTTONS_RX_PART)


// Button Service
#define BUTTONS_START_HDL               0x0800
#define BUTTONS_END_HDL                 (BUTTONS_MAX_HDL - 1)

/* Button Service Handles */
enum
{
  BUTTONS_SVC_HDL = BUTTONS_START_HDL,   /* BUTTONS service declaration */
  BUTTONS_RX_CH_HDL,                     /* BUTTONS RX characteristic */
  BUTTONS_RX_HDL,                        /* BUTTONS write command data */
  BUTTONS_MAX_HDL
};

//*****************************************************************************
//
// Function definitions.
//
//*****************************************************************************
void SvcButtonsAddGroup(void);
void SvcButtonsRemoveGroup(void);
void SvcButtonsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

#ifdef __cplusplus
}
#endif

#endif // SVC_BUTTONS_H
