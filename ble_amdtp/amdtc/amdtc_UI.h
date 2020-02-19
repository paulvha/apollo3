// ****************************************************************************
//
//
//! @file  amdtp_UI.h
//!
//! @brief This file provides header for the client user interface for the amdtc service
//!
//! This has been created and tested to work against Apollo3-board running ble_amdts.ino
//!
//! paulvha / February 2020 / version 1.0
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

#ifndef AMDTP_UI
#define AMDTP_UI

/**
 * command to exchange between client and server
 *
 * This list must stay aligned with the server (BLE_amdpts)
 * it is located in BLE_amdtp.h */
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
    AMDTP_CMD_CUSTOM1,
    AMDTP_CMD_CUSTOM2,
    AMDTP_CMD_CUSTOM3,
    AMDTP_CMD_CUSTOM4,
    AMDTP_CMD_CUSTOM5,
    AMDTP_CMD_MAX
}eAmdtpPktcmd_t;

#define MAX_BUF 50

/* needed for conversion float IEE754 */
typedef union {
    uint8_t array[4];
    float value;
} ByteToFloat;

// foreward declarations
void DetermineValue(gboolean get);
void MainLoop(uint8_t *buf, uint16_t len);

// foreward internal use
void display_menu();
gboolean read_interactive();
float byte_to_float(uint8_t *buf, int x);
void display_BME280(uint8_t *buf, uint16_t len);
void set_terminal();
void rst_terminal();
int rd_keyboard();
void set_conio_terminal_mode();
gboolean validate_adc(int ch);
gboolean validate_pin(int pin);

#endif // AMDTP_UI
