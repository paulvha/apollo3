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

/**
 * paulvha / February 2020 / version 1.0
 *
 * Based on the original amdtp_common.c, this has been changed and extended
 * to work with the Bluez Bluetooth stack on linux
 *
 * This has been created and tested to work against Apollo3-board running ble_amdts.ino
 */

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


#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "amdtp_common.h"
#include "../amdtc_UI.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "lib/bluetooth.h"
#include "lib/hci.h"
#include "lib/hci_lib.h"
#include "lib/sdp.h"
#include "lib/uuid.h"

#include "../att.h"                                // error codes
#include "../btio/btio.h"
#include "../gattrib.h"
#include "../gatt.h"
#include "../amdtc.h"
#include "crc32.h"
#include <unistd.h>

extern void MainLoop(uint8_t *buf, uint16_t len);       // user level MainLoop in amdtc_UI.c
extern uint8_t g_debug;             // Debug level from command line
gboolean CallMain = FALSE;          // used to indicate that MainLoop needs to be called

extern gboolean opt_start_data;

// save received data for processing in user interface
uint8_t RxBuf[AMDTP_MAX_PAYLOAD_SIZE];
uint16_t RxLen;

//*****************************************************************************
//
// Reset a package
//
//*****************************************************************************
void
resetPkt(amdtpPacket_t *pkt)
{
    pkt->offset = 0;
    pkt->header.pktType = AMDTP_PKT_TYPE_UNKNOWN;
    pkt->len = 0;
}

//*****************************************************************************
//
// initialise the structures
//
//*****************************************************************************
gboolean amdtc_init(amdtpCb_t *amdtpCb)
{
    memset(amdtpCb, 0, sizeof(amdtpCb));
    amdtpCb->txReady = false;
    amdtpCb->txState = AMDTP_STATE_INIT;
    amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
    amdtpCb->LastRxPktType = AMDTP_PKT_TYPE_UNKNOWN;

    amdtpCb->lastRxPktSn = 0;                  // keep track of serial number 0 .. 15
    amdtpCb->txPktSn = 0;
    amdtpCb->AckTime = 0;

    resetPkt(&amdtpCb->rxPkt);
    resetPkt(&amdtpCb->txPkt);
    resetPkt(&amdtpCb->ackPkt);

     // allocate memory
    amdtpCb->rxPkt.data = g_try_malloc(AMDTP_PACKET_SIZE);
    if (amdtpCb->rxPkt.data == NULL)    return FALSE;

    amdtpCb->txPkt.data = g_try_malloc(AMDTP_PACKET_SIZE);
    if (amdtpCb->txPkt.data == NULL)    return FALSE;

    amdtpCb->ackPkt.data = g_try_malloc(ATT_DEFAULT_PAYLOAD_LEN);
    if (amdtpCb->ackPkt.data == NULL)   return FALSE;

    return TRUE;
}

//*****************************************************************************
//
// build a packet to sent
//
// amdtpCb   : control block
// type      : data, ack or control
// encrypted : encryption or not
// enableAck : ???????? (is not checked anywhere yet)
// buf       : data to sent
// len       : length of data to sent
//
// testing different ACK response. Now setting enableAck flag
//*****************************************************************************

void
AmdtpBuildPkt(amdtpCb_t *amdtpCb, eAmdtpPktType_t type, gboolean encrypted, gboolean enableACK, uint8_t *buf, uint16_t len)
{
    uint16_t header = 0;
    uint32_t calDataCrc;
    amdtpPacket_t *pkt;

    // select packet to initialise to be sent
    if (type == AMDTP_PKT_TYPE_DATA)
    {
        pkt = &amdtpCb->txPkt;
        resetPkt(pkt);

        // set serial number
        header = amdtpCb->txPktSn << PACKET_SN_BIT_OFFSET;
    }
    // else an ackownledgement or command
    else
    {
        pkt = &amdtpCb->ackPkt;
        resetPkt(pkt);
    }

    // remember the type to use in sendpacket handler
    amdtpCb->TxPktType = type;

    //
    // Prepare header frame to be sent first
    //

    // length
    pkt->len = len + AMDTP_PREFIX_SIZE_IN_PKT + AMDTP_CRC_SIZE_IN_PKT;
    pkt->data[0]  = (len + AMDTP_CRC_SIZE_IN_PKT) & 0xff;
    pkt->data[1]  = ((len + AMDTP_CRC_SIZE_IN_PKT) >> 8) & 0xff;

    // header
    header = header | (type << PACKET_TYPE_BIT_OFFSET);

    if (encrypted)
    {
        header = header | (1 << PACKET_ENCRYPTION_BIT_OFFSET);
    }

    if (enableACK)    // test different ACK apporach
    {
        header = header | (1 << PACKET_ACK_BIT_OFFSET);
    }
    pkt->data[2] = (header & 0xff);
    pkt->data[3] = (header >> 8);

    // copy data
    memcpy(&(pkt->data[AMDTP_PREFIX_SIZE_IN_PKT]), buf, len);
    calDataCrc = CalcCrc32(0xFFFFFFFFU, (uint32_t) len, buf);

    // add checksum
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len] = (calDataCrc & 0xff);
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 1] = ((calDataCrc >> 8) & 0xff);
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 2] = ((calDataCrc >> 16) & 0xff);
    pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 3] = ((calDataCrc >> 24) & 0xff);

    //**************************************************************
    // As we sent to String Characteristic on Artemis, we can not afford zero in the middle
    // we replace with a 0x7E + 0x20 (hopefully unique enough)
    // to be decoded on receipt first as this can also impact the CRC
    // paulvha October 2020
    uint8_t s_value[512];               //AMDTP_PACKET_SIZE
    size_t s_vlen = 0;
    uint16_t l;
    uint16_t i;

    for (l = 0; l < pkt->len; l++) {
        if (pkt->data[l] == 0x0) {
            s_value[s_vlen++] = 0x7E;  // SPECIAL
            s_value[s_vlen++] = 0x20;  // SPECIAL
        }
        else
        {
            s_value[s_vlen++] = pkt->data[l];
        }
    }

    if (g_debug > 0) {
        g_print("============== Prepared for sending ========================\n");
        for ( i =0; i < pkt->len; i++) g_print("0x%02X ", pkt->data[i]);
        g_print("\n");
    }
    //**************************************************************

    // copy back to data
    pkt->len = 0;
    for ( i = 0; i < s_vlen; i++) pkt->data[pkt->len++] = s_value[i];

    if (g_debug > 0){
        g_print("============== Decoded sending ========================\n");
        for ( i =0; i < pkt->len; i++) g_print("0x%02X ", pkt->data[i]);
        g_print("\n");
    }
}

/* timeout check on timely ACK responds
 * check whether the last ACK responds is waiting longer than 2 seconds
 */
guint timeout_ACK(void *data)
{
    amdtpCb_t *amdtpCb = data;
    time_t seconds;

    seconds = time(NULL);

    // if more than 2 seconds passed
    if (seconds - amdtpCb->AckTime > 2) {
        g_printerr("ACK responds failed\n");
        got_error = TRUE;
        g_main_loop_quit(event_loop);
    }

    // come back again later
    return TRUE;
}

//*****************************************************************************
//
// sent a as much of a packet as allowed by the MTU size
//
// amdtpCb : control block
// callback : callback function (if zero this is a repeat call and use earlier saved value)
//
//*****************************************************************************
void
AmdtpSendPacketHandler(amdtpCb_t *amdtpCb, GAttribResultFunc callback)
{
    uint16_t transferSize = 0;
    uint16_t remainingBytes = 0, l;
    amdtpPacket_t *txPkt;
    GAttribResultFunc callbacksave;         // save in case of resend request

    // select the packet to sent (type is set in AmdtpBuildPkt)
    if (amdtpCb->TxPktType == AMDTP_PKT_TYPE_DATA) txPkt = &amdtpCb->txPkt;
    else txPkt = &amdtpCb->ackPkt;

    if (amdtpCb->txState == AMDTP_STATE_TX_IDLE)
    {
        txPkt->offset = 0;
        amdtpCb->txState = AMDTP_STATE_SENDING;
        callbacksave = callback;
    }

    if (amdtpCb->txState == AMDTP_STATE_SENDING)
    {
        // determine how much data can be sent
        remainingBytes = txPkt->len - txPkt->offset;
        transferSize = ((amdtpCb->attMtuSize - 3) > remainingBytes)
                                            ? remainingBytes
                                            : (amdtpCb->attMtuSize - 3);

        if (g_debug > 0){
            g_print(" ===== sending  =====\n");
            for (l = 0; l < transferSize;l++) {
                g_print("0x%x ",txPkt->data[txPkt->offset + l]);
            }
            g_print("\n");
        }

        // send packet
        gatt_write_char(b_attrib, amdtpCb->TX_handle, &txPkt->data[txPkt->offset], transferSize, (GAttribResultFunc) callback, NULL);
        txPkt->offset += transferSize;
    }
    // all has been sent.. now first get an ACK
    if ( txPkt->offset >= txPkt->len )
    {
        // reset the acknowledgement package
        if (amdtpCb->TxPktType != AMDTP_PKT_TYPE_DATA)
            resetPkt(&amdtpCb->ackPkt);

        // set timeout for 2 seconds
        amdtpCb->AckTime = time(NULL);
        g_timeout_add_seconds (2, (GSourceFunc) timeout_ACK, amdtpCb);

        // we can still add new request
        amdtpCb->txState = AMDTP_STATE_TX_IDLE;
    }
}

//*****************************************************************************
//
// AMDTP packet handler
//
// interprete actions to perform from a parsed received package
//
//*****************************************************************************
void
AmdtpPacketHandler(amdtpCb_t *amdtpCb)
{
    uint16_t i;

    switch(amdtpCb->LastRxPktType)
    {
        case AMDTP_PKT_TYPE_DATA:

            //
            // data package received
            //
            // record packet serial number
            amdtpCb->lastRxPktSn = amdtpCb->rxPkt.header.pktSn;

            AmdtpSendReply(amdtpCb, AMDTP_STATUS_SUCCESS, NULL, 0,FALSE);

            // save the data  & len for later
            RxLen = amdtpCb->rxPkt.len;

            // remove CRC
            if (RxLen > 4) RxLen -= 4;

            for (i = 0; i < RxLen; i++)
                RxBuf[i] = amdtpCb->rxPkt.data[i];

            // once called back from ACK we then call MainLoop in commAck_cb
            CallMain=TRUE;

            amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
            resetPkt(&amdtpCb->rxPkt);
            break;

        case AMDTP_PKT_TYPE_ACK:
        {
            if (g_debug > 0) g_print("AMDTP_PKT_TYPE_ACK\n");

            eAmdtpStatus_t status = (eAmdtpStatus_t)amdtpCb->ackPkt.data[0];

            amdtpCb->txState = AMDTP_STATE_TX_IDLE;

            if (status == AMDTP_STATUS_CRC_ERROR || status == AMDTP_STATUS_RESEND_REPLY)
            {
                if (g_debug > 0) g_print("Resending packet\n");
                // resend packet
                AmdtpSendPacketHandler(amdtpCb, NULL);
            }
            else
            {
                // increase packet serial number if send successfully
                if (status == AMDTP_STATUS_SUCCESS)
                {
                    amdtpCb->txPktSn++;
                    if (amdtpCb->txPktSn == 16) amdtpCb->txPktSn = 0;
                }

                // reset packet
                resetPkt(&amdtpCb->txPkt);

                // inform application layer of ACK received with status
                //if (amdtpCb->transCback)
                //{
                //    amdtpCb->transCback(status);
                //}
            }
            resetPkt(&amdtpCb->ackPkt);
        }
            break;

        case AMDTP_PKT_TYPE_CONTROL:
        {
            eAmdtpControl_t control = (eAmdtpControl_t)amdtpCb->ackPkt.data[0];
            uint8_t resendPktSn = amdtpCb->ackPkt.data[1];       // get the serial number to reset from

            if (control == AMDTP_CONTROL_RESEND_REQ)
            {
                amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
                resetPkt(&amdtpCb->rxPkt);
                if (resendPktSn > amdtpCb->lastRxPktSn)
                {
                    AmdtpSendReply(amdtpCb, AMDTP_STATUS_RESEND_REPLY, NULL, 0, TRUE);
                }
                else if (resendPktSn == amdtpCb->lastRxPktSn)
                {
                    AmdtpSendReply(amdtpCb, AMDTP_STATUS_SUCCESS, NULL, 0, FALSE);
                }
                else
                {
                    g_print("ResendPktSn = %d, lastRxPktSn = %d", resendPktSn, amdtpCb->lastRxPktSn);
                }
            }
            else
            {
                g_printerr("unexpected contrl = %d\n", control);
            }
            resetPkt(&amdtpCb->ackPkt);
        }
            break;

        default:
            break;
    }

    amdtpCb->LastRxPktType = AMDTP_PKT_TYPE_UNKNOWN;
}

//*****************************************************************************
//
// RECEIVE a complete package
//
// amdtpCb : control block
// pkt     : rxpkt, ackpacket
// len     : length of received data in pvalue
// pvalue  : received data
//
//*****************************************************************************
eAmdtpStatus_t
AmdtpReceivePkt(amdtpCb_t *amdtpCb, amdtpPacket_t *pkt, uint16_t len, uint8_t *pValue)
{
    uint8_t dataIdx = 0;
    uint32_t calDataCrc = 0;
    uint16_t header = 0;
    uint16_t i;

    // if first of (potential) more, the length must at least be header
    if (pkt->offset == 0 && len < AMDTP_PREFIX_SIZE_IN_PKT)
    {
        g_printerr("Invalid packet size !!!\n");
        AmdtpSendReply(amdtpCb, AMDTP_STATUS_INVALID_PKT_LENGTH, NULL, 0, FALSE);
        return AMDTP_STATUS_INVALID_PKT_LENGTH;
    }

    // new packet
    if (pkt->offset == 0)
    {
        BYTES_TO_UINT16(pkt->len, pValue);
        BYTES_TO_UINT16(header, &pValue[2]);
        pkt->header.pktType = (header & PACKET_TYPE_BIT_MASK) >> PACKET_TYPE_BIT_OFFSET;
        pkt->header.pktSn = (header & PACKET_SN_BIT_MASK) >> PACKET_SN_BIT_OFFSET;
        pkt->header.encrypted = (header & PACKET_ENCRYPTION_BIT_MASK) >> PACKET_ENCRYPTION_BIT_OFFSET;
        pkt->header.ackEnabled = (header & PACKET_ACK_BIT_MASK) >> PACKET_ACK_BIT_OFFSET;
        amdtpCb->LastRxPktType = pkt->header.pktType;
        dataIdx = AMDTP_PREFIX_SIZE_IN_PKT;         // skip size + header in first packaet

        if (pkt->header.pktType == AMDTP_PKT_TYPE_DATA)
        {
            amdtpCb->rxState = AMDTP_STATE_GETTING_DATA;
        }

        if (g_debug > 1) {
            g_print("pkt len = 0x%x\n", pkt->len);
            g_print("pkt header = 0x%x : ", header);
            g_print("type = %d, sn = %d, ", pkt->header.pktType, pkt->header.pktSn);
            g_print("enc = %d, ackEnabled = %d\n", pkt->header.encrypted,  pkt->header.ackEnabled);
        }
    }

    // make sure we have enough space for new data
    if (pkt->offset + len - dataIdx > AMDTP_PACKET_SIZE)
    {
        g_printerr("Not enough buffer size!!!");

        if (pkt->header.pktType == AMDTP_PKT_TYPE_DATA)
        {
            amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
        }
        // reset pkt
        resetPkt(pkt);

        AmdtpSendReply(amdtpCb, AMDTP_STATUS_INSUFFICIENT_BUFFER, NULL, 0,FALSE);
        return AMDTP_STATUS_INSUFFICIENT_BUFFER;
    }

    // copy new data into buffer and also save crc into it it's the last frame in a packet
    // 4 bytes crc is included in pkt length
    memcpy(pkt->data + pkt->offset, pValue + dataIdx, len - dataIdx);
    pkt->offset += (len - dataIdx);

    // whole packet received
    if (pkt->offset >= pkt->len)
    {
        if (g_debug > 0){
            g_print("============== Total Packet received ========================\n");
            for (i =0; i < pkt->len; i++) g_print("0x%02X ", pkt->data[i]);
            g_print("\n");
        }

        uint32_t peerCrc = 0;
        //
        // check CRC
        //
        BYTES_TO_UINT32(peerCrc, pkt->data + pkt->len - AMDTP_CRC_SIZE_IN_PKT);
        calDataCrc = CalcCrc32(0xFFFFFFFFU, pkt->len - AMDTP_CRC_SIZE_IN_PKT, pkt->data);

        if (peerCrc != calDataCrc)
        {
            g_printerr("crc error\n");

            if (g_debug > 1) {
                g_print("calDataCrc = 0x%x, ", calDataCrc);
                g_print("peerCrc = 0x%x, ", peerCrc);
                g_print("len: %d\n", pkt->len);
            }

            if (pkt->header.pktType == AMDTP_PKT_TYPE_DATA)
            {
                amdtpCb->rxState = AMDTP_STATE_RX_IDLE;
            }
            // reset pkt
            resetPkt(pkt);

            AmdtpSendReply(amdtpCb, AMDTP_STATUS_CRC_ERROR, NULL, 0,FALSE);

            return AMDTP_STATUS_CRC_ERROR;
        }

        return AMDTP_STATUS_RECEIVE_DONE;
    }

    return AMDTP_STATUS_RECEIVE_CONTINUE;
}

//*****************************************************************************
//
// Send Reply to Sender
//
//*****************************************************************************
void
AmdtpSendReply(amdtpCb_t *amdtpCb, eAmdtpStatus_t status, uint8_t *data, uint16_t len,gboolean enableACK)
{
    if (g_debug > 1)  g_print("%s\n", __func__);

    uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN] = {0};
    eAmdtpStatus_t st;

    if (len > ATT_DEFAULT_PAYLOAD_LEN) len = ATT_DEFAULT_PAYLOAD_LEN;

    buf[0] = status;

    // add any optional data
    if (len > 0)  memcpy(buf + 1, data, len);

    st = amdtpcSendAck(amdtpCb,AMDTP_PKT_TYPE_ACK, false, enableACK, buf, len + 1);

    if (st != AMDTP_STATUS_SUCCESS)
    {
        g_printerr("amdtpcSendAck Error. Status = %d\n", status);
    }
}

//*****************************************************************************
//
// Call back for acknowledgement sent
//
//*****************************************************************************
static void commACK_cb(guint8 status, const guint8 *pdu, guint16 plen,
                            gpointer user_data)
{
    if (g_debug > 1)  g_print("%s\n", __func__);

    if (status != 0) {
        g_printerr("Sending ACK failed: %s\n", att_ecode2str(status));
        goto error;
    }

    // set during amdtpPacketHandler to indicate data has been
    // received that needs to be processed by the user interface

    if (CallMain){
        CallMain = FALSE;
        MainLoop(RxBuf, RxLen);
        RxLen = 0;
    }
    return;

error:
    got_error = TRUE;
    g_main_loop_quit(event_loop);
}

//*****************************************************************************
//
// Send acknowledgement to server
//
// amdtpCb   : control block
// type      : data, ack or control
// encrypted : encryption or not
// enableAck : ???????? (is not used anywhere yet)
// buf       : data to sent
// len       : length of data to sent
//
//*****************************************************************************
static eAmdtpStatus_t
amdtpcSendAck(amdtpCb_t *amdtpCb, eAmdtpPktType_t type, gboolean encrypted, gboolean enableACK, uint8_t *buf, uint16_t len)
{
    if (g_debug > 1)  g_print("%s\n", __func__);

    //uint16_t handle = amdtpCb->ACK_handle;    // Avoid sending over notify and sent over standard TX to improve stability
    uint16_t handle = amdtpCb->TX_handle;

    AmdtpBuildPkt(amdtpCb, type, encrypted, enableACK, buf, len);

    if (gatt_write_char(b_attrib, handle, amdtpCb->ackPkt.data,amdtpCb->ackPkt.len, commACK_cb, NULL) == 0) {
        g_printerr("AmdtpcSendAck() error during sending\n");
        return AMDTP_STATUS_TX_NOT_READY;
    }

    return AMDTP_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Send control message to Receiver
//
//*****************************************************************************
void
AmdtpSendControl(amdtpCb_t *amdtpCb, eAmdtpControl_t control, uint8_t *data, uint16_t len)
{
    if (g_debug > 1)  g_print("%s\n", __func__);
    uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN] = {0};
    eAmdtpStatus_t st;

    if (len > ATT_DEFAULT_PAYLOAD_LEN) len = ATT_DEFAULT_PAYLOAD_LEN;

    buf[0] = control;
    if (len > 0) memcpy(buf + 1, data, len);

    st = amdtpcSendAck(amdtpCb,AMDTP_PKT_TYPE_CONTROL, false, false, buf, len + 1);

    if (st != AMDTP_STATUS_SUCCESS)
    {
        g_printerr("AmdtpSendControl status = %d\n", st);
    }
}
