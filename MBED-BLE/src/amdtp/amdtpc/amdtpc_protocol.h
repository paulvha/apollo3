// ****************************************************************************
//
//  amdtpc_protocol.h
//! @file
//!
//! @brief This file provides the shared functions for the AMDTP CLIENT service.
//!
//! @{
//
// ****************************************************************************

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

#ifndef AMDTPC_COMMON_H
#define AMDTPC_COMMON_H

#include "../amdtp_common.h"
#include "ble/BLE.h"

/* BLE_DEBUG will show the routines and debug information
 * AMDTP_SHOW_DATA will show the data being received and sent
 *
 * Remove // to enable
 */
//#define AMDTPC_Debug 1
//#define AMDTPC_SHOW_DATA 1

// Server version
#define MAJOR_SERVERVERSION 1         // new features implemented that require update to client
#define MINOR_SERVERVERSION 0         // bug fixes, better calculation / layout

//*****************************************************************************
//
// function definitions
//
//*****************************************************************************
class AMDTPC
{
  public:

    AMDTPC();

   /**
     * initialize the amdtpc protocol stack
     */
    void amdtpc_init();

    /**
     * user program to store any data received from connected device
     */
    eAmdtpStatus_t AmdtpReceivePkt(uint8_t handle, uint16_t len, uint8_t *pValue);

    /**
     * user program to store data to be send to connected device
     * the length is AMDTP_MAX_PAYLOAD_SIZE.
     * The data can / will be split in different packages by server
     * and combined by receiver
     *
     * return
     * -1  error
     *  0  sending of package has been completed
     *  1  Need to return to send next chunk of data (call AmdtpContSendData())
     */
    int AmdtpSendData(uint8_t *buf, uint16_t len);

    /**Check that sending has been completed
     *
     * return
     * True : complete
     * False : not completed
     */
    bool AmdtpSendComplete();

    /**
     * must be called by user program to set a routine to receive and handle
     * the complete received data
     */
    void on_data_received(mbed::Callback<void(uint8_t *data, uint16_t len)> cb);

    /**
     * set call back for sending formatted ACK to server
     */
    void on_ACK_write(mbed::Callback<void(uint8_t *data, uint16_t len)> cb);

    /**
     * set call back for sending formatted data to server
     */
    void on_data_write(mbed::Callback<void(uint8_t *data, uint16_t len)> cb);

    /**
     *  if new MTU size has been agreed with Server / peripheral
     */
    void UpdateMTU(uint16_t newSize);

private:

    bool Check_type(uint16_t len, uint8_t *pValue);

    void AmdtpBuildPkt(eAmdtpPktType_t type, bool encrypted, bool enableACK, uint8_t *buf, uint16_t len);

    void AmdtpSendReply( eAmdtpStatus_t status, uint8_t *data, uint16_t len);

    void AmdtpSendControl( eAmdtpControl_t control, uint8_t *data, uint16_t len);

    void AmdtpSendPacketHandler();

    void AmdtpPacketHandler(eAmdtpPktType_t type, uint16_t len, uint8_t *buf);

    void resetPkt(amdtpPacket_t *pkt);

    void amdtpsSendAck(eAmdtpPktType_t type, bool encrypted, bool enableACK, uint8_t *buf, uint16_t len);

    eAmdtpStatus_t AmdtpsSendPacket(eAmdtpPktType_t type, bool encrypted, bool enableACK, uint8_t *buf, uint16_t len);

    uint8_t rxPktBuf[AMDTP_PACKET_SIZE];        // about 512 + 4 + 4
    uint8_t txPktBuf[AMDTP_PACKET_SIZE];
    uint8_t txDecode[AMDTP_PACKET_SIZE];
    uint8_t ackPktBuf[AMDTP_ACK_SIZE];

    // store the user instructed function for call back
    mbed::Callback<void(uint8_t *data, uint16_t len)> _on_data_cb;

    // in case as running as CLIENT, call back to write formatted ACK data to server
    mbed::Callback<void(uint8_t *data, uint16_t len)> _on_data_ACK_cb;

    // in case as running as CLIENT, call back to write formatted data to server
    mbed::Callback<void(uint8_t *data, uint16_t len)> _on_data_write_cb;

    bool SendingNOtComplete = false;

    uint8_t NewChunkReceived;
};

#endif // AMDTPC_COMMON_H
