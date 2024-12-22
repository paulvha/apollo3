// ****************************************************************************
//
//  amdtp_common.c
//! @file
//!
//! @brief This file provides the shared functions for the AMDTP service.
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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "amdtp_common.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "bstream.h"
#include "att_api.h"
#include "am_util_debug.h"
#include "crc32.h"
#include "am_util.h"

#include "debug.h"

#if (defined BLE_Debug) || (defined BLE_SHOW_DATA)
extern void debug_print(const char* f, const char* F, uint16_t L);
extern void debug_printf(char* fmt, ...);
extern void debug_float (float f);
#endif

#if (defined BLE_Debug)
#define AMDTP_DEBUG_ON 1
#endif

extern eAmdtpStatus_t AmdtpsSendPacket(eAmdtpPktType_t type, bool_t encrypted, bool_t enableACK, uint8_t *buf, uint16_t len);

void
resetPkt(amdtpPacket_t *pkt)
{
    pkt->offset = 0;        // no data in buffer to start from
    pkt->header.pktType = AMDTP_PKT_TYPE_UNKNOWN; // unknown what is left
    pkt->len = 0;           // no length in buffer
}

/* parse a received message
 * AmdtpReceivePkt(&amdtpsCb.core, &amdtpsCb.core.rxPkt, len, pValue);
 */
eAmdtpStatus_t
AmdtpReceivePkt(amdtpCb_t *amdtpCb, amdtpPacket_t *pkt, uint16_t len, uint8_t *pValue)
{
   #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif

    uint8_t dataIdx = 0;
    uint32_t calDataCrc = 0;
    uint16_t header = 0;
    // len must include length (2) and header (2)
    if (pkt->offset == 0 && len < AMDTP_PREFIX_SIZE_IN_PKT)
    {
        #ifdef BLE_Debug
          debug_printf("Invalid packet!!!");
        #endif
        AmdtpSendReply(amdtpCb, AMDTP_STATUS_INVALID_PKT_LENGTH, NULL, 0);
        return AMDTP_STATUS_INVALID_PKT_LENGTH;
    }

    // new packet (not received packet before that was copied in)
    if (pkt->offset == 0)
    {
        BYTES_TO_UINT16(pkt->len, pValue);      // first 2 bytes is length
        BYTES_TO_UINT16(header, &pValue[2]);    // second 2 bytes is header
        pkt->header.pktType = (header & PACKET_TYPE_BIT_MASK) >> PACKET_TYPE_BIT_OFFSET; // mask = 0xf, offset is 12 : top 4 bits
        pkt->header.pktSn = (header & PACKET_SN_BIT_MASK) >> PACKET_SN_BIT_OFFSET;       // mask = 0xf , ofsset is 8 : next for 4 bits
        pkt->header.encrypted = (header & PACKET_ENCRYPTION_BIT_MASK) >> PACKET_ENCRYPTION_BIT_OFFSET;  // mask = 1, offset is 7, next bit
        pkt->header.ackEnabled = (header & PACKET_ACK_BIT_MASK) >> PACKET_ACK_BIT_OFFSET; // mask 1, offset is 6, next bit
        dataIdx = AMDTP_PREFIX_SIZE_IN_PKT;     // 4
        if (pkt->header.pktType == AMDTP_PKT_TYPE_DATA) // if data
        {
            amdtpCb->rxState = AMDTP_STATE_GETTING_DATA;    // indicate getting data
        }

#ifdef BLE_SHOW_DATA
        debug_printf("pkt len = 0x%x, pkt header = 0x%x :\n", pkt->len, header);
        switch (pkt->header.pktType) {
            case  AMDTP_PKT_TYPE_DATA:
                debug_printf("type = %d : DATA ", pkt->header.pktType);
                break;
            case  AMDTP_PKT_TYPE_ACK:
                debug_printf("type = %d : ACK ", pkt->header.pktType);
                break;
            case  AMDTP_PKT_TYPE_CONTROL:
                debug_printf("type = %d : Control ", pkt->header.pktType);
                break;
            default:
                debug_printf("type = %d : Unknown ", pkt->header.pktType);
                break;
        }
        debug_printf("sn = %d, enc = %d, ackEnabled = %d\n",pkt->header.pktSn, pkt->header.encrypted,  pkt->header.ackEnabled);
#endif
    }

    // make sure we have enough space for new data
    if (pkt->offset + len - dataIdx > AMDTP_PACKET_SIZE)    // make sure it fits in 512 + 4 +4 (max length defined amdtp_common.h:  512)
    {
#ifdef BLE_Debug
          debug_printf("not enough buffer size!!!\n");
#endif
        if (pkt->header.pktType == AMDTP_PKT_TYPE_DATA)     // if data
        {
            amdtpCb->rxState = AMDTP_STATE_RX_IDLE;         // set to idle
        }
        // reset pkt
        resetPkt(pkt);
        AmdtpSendReply(amdtpCb, AMDTP_STATUS_INSUFFICIENT_BUFFER, NULL, 0);
        return AMDTP_STATUS_INSUFFICIENT_BUFFER;
    }

    // copy new data into buffer and also save crc into it if it's the last frame in a packet
    // 4 bytes crc is included in pkt length
    memcpy(pkt->data + pkt->offset, pValue + dataIdx, len - dataIdx); // skip the header
    pkt->offset += (len - dataIdx);

    // whole packet received
    if (pkt->offset >= pkt->len)
    {
        uint32_t peerCrc = 0;
        //
        // check CRC
        //
        BYTES_TO_UINT32(peerCrc, pkt->data + pkt->len - AMDTP_CRC_SIZE_IN_PKT);
        calDataCrc = CalcCrc32(0xFFFFFFFFU, pkt->len - AMDTP_CRC_SIZE_IN_PKT, pkt->data);

        if (peerCrc != calDataCrc)
        {
#ifdef BLE_Debug
            debug_printf("crc error :\ncalDataCrc = 0x%x ", calDataCrc);
            debug_printf("peerCrc = 0x%x\n", peerCrc);
#endif
            if (pkt->header.pktType == AMDTP_PKT_TYPE_DATA)
            {
                amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
            }
            // reset pkt
            resetPkt(pkt);

            AmdtpSendReply(amdtpCb, AMDTP_STATUS_CRC_ERROR, NULL, 0);

            return AMDTP_STATUS_CRC_ERROR;
        }

        return AMDTP_STATUS_RECEIVE_DONE;
    }

    // not all had been received
    return AMDTP_STATUS_RECEIVE_CONTINUE;
}

//*****************************************************************************
//
// AMDTP packet handler
//
//*****************************************************************************
void
AmdtpPacketHandler(amdtpCb_t *amdtpCb, eAmdtpPktType_t type, uint16_t len, uint8_t *buf)
{

   #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
   #endif

    switch(type)
    {
        case AMDTP_PKT_TYPE_DATA:
            //
            // data package recevied
            //
            // record packet serial number
            amdtpCb->lastRxPktSn = amdtpCb->rxPkt.header.pktSn;
            AmdtpSendReply(amdtpCb, AMDTP_STATUS_SUCCESS, NULL, 0);
            if (amdtpCb->recvCback)     // if callback on received data, call it now
            {
                amdtpCb->recvCback(buf, len);   // amdtpDtpRecvCback
            }

            amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
            resetPkt(&amdtpCb->rxPkt);
            break;

        case AMDTP_PKT_TYPE_ACK:
        {
            eAmdtpStatus_t status = (eAmdtpStatus_t)buf[0];

            // stop tx timeout timer
            WsfTimerStop(&amdtpCb->timeoutTimer);

            amdtpCb->txState = AMDTP_STATE_TX_IDLE;

            if (status == AMDTP_STATUS_CRC_ERROR || status == AMDTP_STATUS_RESEND_REPLY)
            {
                // resend packet
                AmdtpSendPacketHandler(amdtpCb);
            }
            else
            {
                // increase packet serial number if send successfully
                if (status == AMDTP_STATUS_SUCCESS)
                {
                    amdtpCb->txPktSn++;
                    if (amdtpCb->txPktSn == 16)     // max 4 bits part of the header (so count 0 - 15)
                    {
                        amdtpCb->txPktSn = 0;
                    }
                }

                // packet transfer successful or other error
                // reset packet
                resetPkt(&amdtpCb->txPkt);

                // notify application layer (transCback checks on sending testdata)
                if (amdtpCb->transCback)    // if transmit call back, let it now
                {
                    amdtpCb->transCback(status);
                }
            }
            resetPkt(&amdtpCb->ackPkt);
        }
            break;

        case AMDTP_PKT_TYPE_CONTROL:
        {
            eAmdtpControl_t control = (eAmdtpControl_t)buf[0];
            uint8_t resendPktSn = buf[1];
            if (control == AMDTP_CONTROL_RESEND_REQ)
            {
                amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
                resetPkt(&amdtpCb->rxPkt);
                if (resendPktSn > amdtpCb->lastRxPktSn)
                {
                    AmdtpSendReply(amdtpCb, AMDTP_STATUS_RESEND_REPLY, NULL, 0);
                }
                else if (resendPktSn == amdtpCb->lastRxPktSn)
                {
                    AmdtpSendReply(amdtpCb, AMDTP_STATUS_SUCCESS, NULL, 0);
                }
                else
                {
                #ifdef BLE_Debug
                   debug_printf("resendPktSn = %d, lastRxPktSn = %d", resendPktSn, amdtpCb->lastRxPktSn);
                #endif
                }
            }
            else
            {
                #ifdef BLE_Debug
                   debug_printf("unexpected contrl = %d\n", control);
                #endif
            }
            resetPkt(&amdtpCb->ackPkt);
        }
            break;

        default:
        break;
    }
}

/* create packet to send */
void
AmdtpBuildPkt(amdtpCb_t *amdtpCb, eAmdtpPktType_t type, bool_t encrypted, bool_t enableACK, uint8_t *buf, uint16_t len)
{
   #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
   #endif
    uint16_t header = 0;
    uint32_t calDataCrc;
    amdtpPacket_t *pkt;

    if (type == AMDTP_PKT_TYPE_DATA)
    {
        pkt = &amdtpCb->txPkt;
        header = amdtpCb->txPktSn << PACKET_SN_BIT_OFFSET;  // add 4 bit serial number to right offset
    }
    else
    {
        pkt = &amdtpCb->ackPkt;                             // sent acknowledgement of earlier received data
    }

    //
    // Prepare header frame to be sent first
    //
    // length
    pkt->len = len + AMDTP_PREFIX_SIZE_IN_PKT + AMDTP_CRC_SIZE_IN_PKT;  // len of data + header & len (4) + CRC (4 ytes)
    pkt->data[0]  = (len + AMDTP_CRC_SIZE_IN_PKT) & 0xff;          // LSB
    pkt->data[1]  = ((len + AMDTP_CRC_SIZE_IN_PKT) >> 8) & 0xff;   // MSB length

    // header
    header = header | (type << PACKET_TYPE_BIT_OFFSET);             // add 4 bit type
    if (encrypted)
    {
        header = header | (1 << PACKET_ENCRYPTION_BIT_OFFSET);      // set encryption bit
    }
    if (enableACK)
    {
        header = header | (1 << PACKET_ACK_BIT_OFFSET);             // set acknowledgement bit
    }
    pkt->data[2] = (header & 0xff);                                 // set header
    pkt->data[3] = (header >> 8);

    // copy data
    memcpy(&(pkt->data[AMDTP_PREFIX_SIZE_IN_PKT]), buf, len);       // now add the data
    calDataCrc = CalcCrc32(0xFFFFFFFFU, len, buf);                  // calculate the CRC

    // add checksum
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len] = (calDataCrc & 0xff); // add the 32 bit CRC
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 1] = ((calDataCrc >> 8) & 0xff);
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 2] = ((calDataCrc >> 16) & 0xff);
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 3] = ((calDataCrc >> 24) & 0xff);

#ifdef BLE_SHOW_DATA
    debug_printf("============== Prepared for sending ========================\n");
    for (uint16_t i =0; i < pkt->len; i++) debug_printf("0x%02X ", pkt->data[i]);
    debug_printf("\n");
#endif
}

//*****************************************************************************
//
// Send data to client
//
//*****************************************************************************
bool AmdtpSendData(uint8_t *buf, uint16_t len)
{
    #ifdef BLE_Debug
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    eAmdtpStatus_t st = AmdtpsSendPacket(AMDTP_PKT_TYPE_DATA, false, false, buf, len);

    if (st != AMDTP_STATUS_SUCCESS)
    {
#ifdef BLE_Debug
        debug_printf("AmdtpSendData failedstatus = %d\n", st);
#endif
        return false;
    }

    return true;
}


//*****************************************************************************
//
// Send Reply (ACK - type with status and optional data) to Sender
//
//*****************************************************************************
void
AmdtpSendReply(amdtpCb_t *amdtpCb, eAmdtpStatus_t status, uint8_t *data, uint16_t len)
{
    #ifdef BLE_Debug
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN] = {0}; // max payload length for most PDU on bluetooth
    eAmdtpStatus_t st;

    WSF_ASSERT(len < ATT_DEFAULT_PAYLOAD_LEN);

    buf[0] = status;                    // first byte is status

    if (len > 0)                        // add any data if needed
    {
        memcpy(buf + 1, data, len);
    }

    st = amdtpCb->ack_sender_func(AMDTP_PKT_TYPE_ACK, false, false, buf, len + 1);

    if (st != AMDTP_STATUS_SUCCESS)
    {
#ifdef BLE_Debug
          debug_printf("AmdtpSendReply failed status = %d\n", st);
#endif
    }
}

//*****************************************************************************
//
// Send control message to Receiver
//
//*****************************************************************************
void
AmdtpSendControl(amdtpCb_t *amdtpCb, eAmdtpControl_t control, uint8_t *data, uint16_t len)
{
   #ifdef BLE_Debug
    debug_print(__func__, __FILE__, __LINE__);
  #endif

    uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN] = {0};
    eAmdtpStatus_t st;

    WSF_ASSERT(len < ATT_DEFAULT_PAYLOAD_LEN);

    buf[0] = control;
    if (len > 0)
    {
        memcpy(buf + 1, data, len);
    }
    st = amdtpCb->ack_sender_func(AMDTP_PKT_TYPE_CONTROL, false, false, buf, len + 1);
    if (st != AMDTP_STATUS_SUCCESS)
    {
   #ifdef BLE_Debug
        debug_printf("AmdtpSendControl failed status = %d\n", st);
   #endif
    }
}

void
AmdtpSendPacketHandler(amdtpCb_t *amdtpCb)
{
    #ifdef BLE_Debug
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    uint16_t transferSize = 0;
    uint16_t remainingBytes = 0;
    amdtpPacket_t *txPkt = &amdtpCb->txPkt;

    if ( amdtpCb->txState == AMDTP_STATE_TX_IDLE )
    {
        txPkt->offset = 0;
        amdtpCb->txState = AMDTP_STATE_SENDING;
    }

    if ( txPkt->offset >= txPkt->len )
    {
        // done sent packet
        amdtpCb->txState = AMDTP_STATE_WAITING_ACK;

        // start tx timeout timer. ACK must arrive within that time
        WsfTimerStartMs(&amdtpCb->timeoutTimer, amdtpCb->txTimeoutMs);
    }
    else
    {
        remainingBytes = txPkt->len - txPkt->offset;
        transferSize = ((amdtpCb->attMtuSize - 3) > remainingBytes)
                                            ? remainingBytes
                                            : (amdtpCb->attMtuSize - 3);
        // send packet
        amdtpCb->data_sender_func(&txPkt->data[txPkt->offset], transferSize);   //amdtpsSendData
        txPkt->offset += transferSize;
    }
}
