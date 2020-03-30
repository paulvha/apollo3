// ****************************************************************************
//
//
//! @file  amdtp_srvcmds.h
//!
//! @brief This file provides header for the client user interface for the amdtc service
//!
//! This has been created and tested to work against Apollo3-board running ble_amdts.ino
//!
//! paulvha / March 2020 / version 1.0
//! @{
//
// ****************************************************************************
//*****************************************************************************
//
// Copyright (c) 2020, Paul van Haastrecht
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
//*****************************************************************************
#ifndef AMDTPC_SVR_H
#define AMDTPC_SVR_H

#include <stdlib.h>     // atoi
#include "wsf_types.h"
#include "amdtpc_api.h"
#include "amdtp_api.h"
#include "ble_menu.h"
#include "amdtp_common.h"

#ifdef __cplusplus
extern "C" {
#endif


//*****************************************************************************
//
// Server commands
//
//*****************************************************************************
typedef enum eAmdtpcmd
{
    AMDTP_CMD_NONE,
    AMDTP_CMD_START_TEST_DATA,
    AMDTP_CMD_STOP_TEST_DATA,
    AMDTP_CMD_HELLO,
    AMDTP_CMD_REQ_BATTERY_LEVEL,
    AMDTP_CMD_REQ_BATTERYLOAD_ON,
    AMDTP_CMD_REQ_BATTERYLOAD_OFF,
    AMDTP_CMD_REQ_INTERNAL_TEMP_CEL,
    AMDTP_CMD_REQ_INTERNAL_TEMP_FRH,
    AMDTP_CMD_TURN_LED_ON,
    AMDTP_CMD_TURN_LED_OFF,
    AMDTP_CMD_BME280,
    AMDTP_CMD_ADC,
    AMDTP_CMD_CHAT,
    AMDTP_CMD_READ_PIN,
    AMDTP_CMD_PIN_HIGH,
    AMDTP_CMD_PIN_LOW,
    AMDTP_CMD_VERSION,
    AMDTP_CMD_CUSTOM1,
    AMDTP_CMD_CUSTOM2,
    AMDTP_CMD_CUSTOM3,
    AMDTP_CMD_CUSTOM4,
    AMDTP_CMD_CUSTOM5,
    AMDTP_CMD_MAX
}eAmdtpPktcmd_t;

// needed for conversion float IEE754
typedef union {
    uint8_t array[4];
    float value;
} ByteToFloat;

//*****************************************************************************
//
// external calls
//
//*****************************************************************************
void handleAMDTPSlection(void);                     // Input from keyboard for menu
void CmdInput();                                    // Pending Input from keyboard for commands
void HandeServerResp(uint8_t * buf, uint16_t len);  // Server responds
eAmdtpPktcmd_t PendingCmdInput;                     // indicator pending commands input

//*****************************************************************************
//
// internal calls
//
//*****************************************************************************
void AmdtpAskPinInput(uint8_t cmd);
void SendReqPin();
void AmdtpReqBatLd();
void SendChat();
void display_BME280(uint8_t *buf, uint16_t len);
float byte_to_float(uint8_t *buf, int x);
eAmdtpStatus_t AmdtpcSendCmd(uint8_t cmd, uint8_t *buf, uint8_t len);

#ifdef __cplusplus
};
#endif

#endif // AMDTPC_SVR_H
