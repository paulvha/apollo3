//*****************************************************************************
//
//  amdtps_main.c
//! @file
//!
//! @brief This file provides the main application for the AMDTP service.
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

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_buf.h"    //for WsfBufAlloc and WsfBufFree
#include "bstream.h"
#include "att_api.h"
#include "svc_ch.h"
#include "svc_amdtp.h"
#include "app_api.h"
#include "app_hw.h"
#include "amdtps_api.h"
#include "am_util_debug.h"
#include "crc32.h"

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include "debug.h"

#if (defined BLE_Debug) || (defined BLE_SHOW_DATA)
extern void debug_print(const char* f, const char* F, uint16_t L);
extern void debug_printf(char* fmt, ...);
extern void debug_float (float f);
#endif

#ifdef BLE_Debug
#define AMDTPS_DEBUG_ON
#endif
//*****************************************************************************
//
// Global variables
//
//*****************************************************************************

uint8_t rxPktBuf[AMDTP_PACKET_SIZE];        // about 2048 + 4 + 4
uint8_t txPktBuf[AMDTP_PACKET_SIZE];
uint8_t ackPktBuf[20];

#if defined(AMDTPS_RXONLY) || defined(AMDTPS_RX2TX)
static int totalLen = 0;
#endif

//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************
//KHE
/* Control block */
static struct
{
    amdtpsConn_t            conn[DM_CONN_MAX];      // connection control block (max 8)
    bool_t                  txReady;                // TRUE if ready to send notifications
    wsfHandlerId_t          appHandlerId;
    AmdtpsCfg_t             cfg;                    // configurable parameters
    amdtpCb_t               core;
}
amdtpsCb;

//*****************************************************************************
//
// Connection Open event
//
//*****************************************************************************
static void
amdtps_conn_open(dmEvt_t *pMsg)
{
    #ifdef AMDTPS_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);

    hciLeConnCmplEvt_t *evt = (hciLeConnCmplEvt_t*) pMsg;

    debug_printf("connection opened\n");
    debug_printf("handle = 0x%X\n", evt->handle);
    debug_printf("role = 0x%X\n", evt->role);
    debug_printf("addrMSB = %02X:%02X:%02X:%02X:%02X:%02X\n", evt->peerAddr[0], evt->peerAddr[1], evt->peerAddr[2]);
    debug_printf("addrLSB = %02X%02X%02x%02X%02X%02X\n", evt->peerAddr[3], evt->peerAddr[4], evt->peerAddr[5]);
    debug_printf("connInterval = 0x%X\n", evt->connInterval);
    debug_printf("connLatency = 0x%X\n", evt->connLatency);
    debug_printf("supTimeout = 0x%X\n", evt->supTimeout);

    #endif
}

//*****************************************************************************
//
// Connection Update event
//
//*****************************************************************************
static void
amdtps_conn_update(dmEvt_t *pMsg)
{
    #ifdef AMDTPS_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);

    hciLeConnUpdateCmplEvt_t *evt = (hciLeConnUpdateCmplEvt_t*) pMsg;

    debug_printf("connection update status = 0x%X", evt->status);
    debug_printf("handle = 0x%X", evt->handle);
    debug_printf("connInterval = 0x%X", evt->connInterval);
    debug_printf("connLatency = 0x%X", evt->connLatency);
    debug_printf("supTimeout = 0x%X", evt->supTimeout);

    #endif
}

static void amdtpsSetupToSend(void)
{
    #ifdef AMDTPS_DEBUG_ON
    //    debug_print(__func__, __FILE__, __LINE__);
    #endif

    amdtpsConn_t    *pConn = amdtpsCb.conn;
    uint8_t       i;

    for (i = 0; i < DM_CONN_MAX; i++, pConn++)
    {
        if (pConn->connId != DM_CONN_ID_NONE)
        {
            pConn->amdtpToSend = true;      // indicate ready to be send on this channel
        }
    }
}

//*****************************************************************************
//
// Find Next Connection to Send on
//
//*****************************************************************************
static amdtpsConn_t*
amdtps_find_next2send(void)
{
    #ifdef AMDTPS_DEBUG_ON
    //    debug_print(__func__, __FILE__, __LINE__);
    #endif

    amdtpsConn_t *pConn = amdtpsCb.conn;    // this assumes there is only one and should check for pConn->amdtpToSend
    return pConn;
}

//*****************************************************************************
//
// Send Notification to Client
// the buffer has already been created/formatted before
//
//*****************************************************************************
static void
amdtpsSendData(uint8_t *buf, uint16_t len)
{
    #ifdef AMDTPS_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    amdtpsSetupToSend();
    amdtpsConn_t *pConn = amdtps_find_next2send();
    /* send notification */
    if (pConn)
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("amdtpsSendData(), Send to connId = %d\n", pConn->connId);
#endif
        AttsHandleValueNtf(pConn->connId, AMDTPS_TX_HDL, len, buf);

        pConn->amdtpToSend = false;
        amdtpsCb.txReady = false;
    }
    else
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("Invalid Conn = %d\n", pConn->connId);
#endif
    }
}

/*
 *   send acknowledgement
 *   type : ack or data or control
 *  encrypted set encryption
 *  enableack ???
 *  buf : data to sent
 *  len : length of data to sent
 */
static eAmdtpStatus_t
amdtpsSendAck(eAmdtpPktType_t type, bool_t encrypted, bool_t enableACK, uint8_t *buf, uint16_t len)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    /* Check still connected */
    if (AppConnIsOpen() == DM_CONN_ID_NONE)
    {
        #ifdef AMDTP_DEBUG_ON
          debug_printf(" not connected\n");
        #endif
        return AMDTP_STATUS_TX_NOT_READY;
    }

    // create packet with CRC etc
    AmdtpBuildPkt(&amdtpsCb.core, type, encrypted, enableACK, buf, len);

    // find free connection ID
    amdtpsSetupToSend();    // not used ..
    amdtpsConn_t *pConn = amdtps_find_next2send();

    /* send notification */
    if (pConn)
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("amdtpsSendAck(), Send to connId = %d\n", pConn->connId);
#endif
        AttsHandleValueNtf(pConn->connId, AMDTPS_ACK_HDL, amdtpsCb.core.ackPkt.len, amdtpsCb.core.ackPkt.data);

        pConn->amdtpToSend = false;
    }
    else
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("NOT sending ACK : Invalid Conn = %d\n", pConn->connId);
#endif
        return AMDTP_STATUS_TX_NOT_READY;
    }

    return AMDTP_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Timer Expiration handler
//
//*****************************************************************************
static void
amdtps_timeout_timer_expired(wsfMsgHdr_t *pMsg)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    /* Check still connected */
    if (AppConnIsOpen() == DM_CONN_ID_NONE)
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("not connected\n");
        debug_printf("amdtps tx timeout, txPktSn = %d", amdtpsCb.core.txPktSn);
#endif
        return;
    }
    uint8_t data[1];
    data[0] = amdtpsCb.core.txPktSn;    // include serial number of time out

    AmdtpSendControl(&amdtpsCb.core, AMDTP_CONTROL_RESEND_REQ, data, 1); // let the client reset

    // fire a timer for receiving an AMDTP_STATUS_RESEND_REPLY ACK
    WsfTimerStartMs(&amdtpsCb.core.timeoutTimer, amdtpsCb.core.txTimeoutMs);
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpsHandleValueCnf
 *
 *  \brief  Handle a received ATT handle value confirm.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void
amdtpsHandleValueCnf(attEvt_t *pMsg)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    //debug_printf("Cnf status = %d, handle = 0x%x\n", pMsg->hdr.status, pMsg->handle);
    if (pMsg->hdr.status == ATT_SUCCESS)
    {
#if !defined(AMDTPS_RXONLY) && !defined(AMDTPS_RX2TX)
        if (pMsg->handle == AMDTPS_TX_HDL)  // if it was data (NOT ACK or Control)
        {   // trigger to continue sending if any data left to sent
            amdtpsCb.txReady = true;
            // process next data
            AmdtpSendPacketHandler(&amdtpsCb.core);

#ifdef AMDTPS_TXTEST
            // fixme when last packet, continue to send next one.
            if (amdtpsCb.core.txState == AMDTP_STATE_WAITING_ACK)
            {
                uint8_t temp[3];
                temp[0] = AMDTP_STATUS_SUCCESS;
                AmdtpPacketHandler(&amdtpsCb.core, AMDTP_PKT_TYPE_ACK, 3, temp);
            }
#endif // AMDTPS_TXTEST
        }
#endif // !defined(AMDTPS_RXONLY) && !defined(AMDTPS_RX2TX)
    }
    else
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("cnf failure  status = %d, hdl = 0x%x\n", pMsg->hdr.status, pMsg->handle);
#endif
    }
}

//*****************************************************************************
//
//! @brief initialize amdtp service
//!
//! @param handlerId - connection handle
//! @param pCfg - configuration parameters
//!
//! @return None
//
//*****************************************************************************
void
amdtps_init(wsfHandlerId_t handlerId, AmdtpsCfg_t *pCfg, amdtpRecvCback_t recvCback, amdtpTransCback_t transCback)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    memset(&amdtpsCb, 0, sizeof(amdtpsCb));
    amdtpsCb.appHandlerId = handlerId;
    amdtpsCb.txReady = false;
    amdtpsCb.core.txState = AMDTP_STATE_INIT;
    amdtpsCb.core.rxState = AMDTP_STATE_RX_IDLE;
    amdtpsCb.core.timeoutTimer.handlerId = handlerId;
    for (int i = 0; i < DM_CONN_MAX; i++)           // there is only  1
    {
        amdtpsCb.conn[i].connId = DM_CONN_ID_NONE;
    }

    amdtpsCb.core.lastRxPktSn = 0;                  // keep track of serial number 0 .. 15
    amdtpsCb.core.txPktSn = 0;

    resetPkt(&amdtpsCb.core.rxPkt);                 // set buffers
    amdtpsCb.core.rxPkt.data = rxPktBuf;

    resetPkt(&amdtpsCb.core.txPkt);
    amdtpsCb.core.txPkt.data = txPktBuf;

    resetPkt(&amdtpsCb.core.ackPkt);
    amdtpsCb.core.ackPkt.data = ackPktBuf;

    amdtpsCb.core.recvCback = recvCback;
    amdtpsCb.core.transCback = transCback;

    amdtpsCb.core.txTimeoutMs = TX_TIMEOUT_DEFAULT;

    amdtpsCb.core.data_sender_func = amdtpsSendData;
    amdtpsCb.core.ack_sender_func = amdtpsSendAck;
}

static void
amdtps_conn_close(dmEvt_t *pMsg)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    hciDisconnectCmplEvt_t *evt = (hciDisconnectCmplEvt_t*) pMsg;
    dmConnId_t connId = evt->hdr.param;
    /* clear connection */
    amdtpsCb.conn[connId - 1].connId = DM_CONN_ID_NONE;
    amdtpsCb.conn[connId - 1].amdtpToSend = FALSE;

    WsfTimerStop(&amdtpsCb.core.timeoutTimer);
    amdtpsCb.core.txState = AMDTP_STATE_INIT;
    amdtpsCb.core.rxState = AMDTP_STATE_RX_IDLE;
    amdtpsCb.core.lastRxPktSn = 0;
    amdtpsCb.core.txPktSn = 0;
    resetPkt(&amdtpsCb.core.rxPkt);
    resetPkt(&amdtpsCb.core.txPkt);
    resetPkt(&amdtpsCb.core.ackPkt);

#if defined(AMDTPS_RXONLY) || defined(AMDTPS_RX2TX)
    #ifdef AMDTP_DEBUG_ON
     debug_printf("*** RECEIVED TOTAL %d ***", totalLen);
    #endif
    totalLen = 0;
#endif

}
/* call back for amdtps write back
 * so when something is written to this device
 */
uint8_t
amdtps_write_cback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                   uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif

    eAmdtpStatus_t status = AMDTP_STATUS_UNKNOWN_ERROR;
    amdtpPacket_t *pkt = NULL;

#ifdef BLE_SHOW_DATA       // debug info
    debug_printf("============= data arrived start ===============\n");
    for (uint16_t i = 0; i < len; i++) debug_printf("0x%x ", pValue[i]);
    debug_printf("\n============= data arrived end ===============\n");
#endif

    if (handle == AMDTPS_RX_HDL)
    {

#if defined(AMDTPS_RX2TX)           // if loopback was set
        amdtpsSendData(pValue, len);
#endif

#if defined(AMDTPS_RXONLY) || defined(AMDTPS_RX2TX)         // only count totals
        totalLen += len;
         #ifdef AMDTP_DEBUG_ON
             debug_print("received data len %d, total %d", len, totalLen);
         #endif
        return ATT_SUCCESS;
#else // RXONLY && RX2TX                                    // do something with the received packet
        status = AmdtpReceivePkt(&amdtpsCb.core, &amdtpsCb.core.rxPkt, len, pValue);
#endif // RXONLY && RX2TX
    }
    else if (handle == AMDTPS_ACK_HDL)
    {
        status = AmdtpReceivePkt(&amdtpsCb.core, &amdtpsCb.core.ackPkt, len, pValue);
    }

    if (status == AMDTP_STATUS_RECEIVE_DONE)
    {
        if (handle == AMDTPS_RX_HDL)
        {
            pkt = &amdtpsCb.core.rxPkt;
        }
        else if (handle == AMDTPS_ACK_HDL)
        {
            pkt = &amdtpsCb.core.ackPkt;
        }

        AmdtpPacketHandler(&amdtpsCb.core, (eAmdtpPktType_t)pkt->header.pktType, pkt->len - AMDTP_CRC_SIZE_IN_PKT, pkt->data);
    }

    return ATT_SUCCESS;
}

void
amdtps_start(dmConnId_t connId, uint8_t timerEvt, uint8_t amdtpCccIdx)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif
    //
    // set conn id
    //
    amdtpsCb.conn[connId - 1].connId = connId;
    amdtpsCb.conn[connId - 1].amdtpToSend = TRUE;

    amdtpsCb.core.timeoutTimer.msg.event = timerEvt;
    amdtpsCb.core.txState = AMDTP_STATE_TX_IDLE;
    amdtpsCb.txReady = true;

    amdtpsCb.core.attMtuSize = AttGetMtu(connId);
#ifdef AMDTP_DEBUG_ON
    debug_printf("MTU size = %d bytes", amdtpsCb.core.attMtuSize);
#endif
}

void
amdtps_stop(dmConnId_t connId)
{
    #ifdef AMDTP_DEBUG_ON
        debug_print(__func__, __FILE__, __LINE__);
    #endif
    //
    // clear connection
    //
    amdtpsCb.conn[connId - 1].connId = DM_CONN_ID_NONE;
    amdtpsCb.conn[connId - 1].amdtpToSend = FALSE;

    amdtpsCb.core.txState = AMDTP_STATE_INIT;
    amdtpsCb.txReady = false;
}

void
amdtps_proc_msg(wsfMsgHdr_t *pMsg)
{
#ifdef AMDTP_DEBUG_ON
    debug_print(__func__, __FILE__, __LINE__);
#endif
    if (pMsg->event == DM_CONN_OPEN_IND)            /*! Connection opened */
    {
        amdtps_conn_open((dmEvt_t *) pMsg); // info only
    }
    else if (pMsg->event == DM_CONN_CLOSE_IND)      /*! Connection closed */
    {
        amdtps_conn_close((dmEvt_t *) pMsg); // reset variables
    }
    else if (pMsg->event == DM_CONN_UPDATE_IND)     /*! Connection update complete */
    {
        amdtps_conn_update((dmEvt_t *) pMsg); // info only
    }
    else if (pMsg->event == amdtpsCb.core.timeoutTimer.msg.event)  /*! AMDTP tx timeout timer expired */
    {
        amdtps_timeout_timer_expired(pMsg);    // sent cancel to client
    }
    else if (pMsg->event == ATTS_HANDLE_VALUE_CNF)  /*!< \brief Handle value confirmation */
    {
        amdtpsHandleValueCnf((attEvt_t *) pMsg);
    }
}

//*****************************************************************************
//
//! @brief Send data to Client via notification
//!
//! @param type - packet type
//! @param encrypted - is packet encrypted
//! @param enableACK - does client need to response
//! @param buf - data
//! @param len - data length
//!
//! @return status
//
//*****************************************************************************
eAmdtpStatus_t
AmdtpsSendPacket(eAmdtpPktType_t type, bool_t encrypted, bool_t enableACK, uint8_t *buf, uint16_t len)
{
#ifdef AMDTP_DEBUG_ON
    debug_print(__func__, __FILE__, __LINE__);
#endif
    //
    // Check if ready to send notification
    //
    if ( !amdtpsCb.txReady )
    {
        // set in callback amdtpsHandleValueCnf
#ifdef AMDTP_DEBUG_ON
    debug_print("data sending failed, not ready for notification.", NULL);
#endif
        return AMDTP_STATUS_TX_NOT_READY;
    }

    //
    // Check if the service is idle to send
    //
    if ( amdtpsCb.core.txState != AMDTP_STATE_TX_IDLE )
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("data sending failed, tx state = %d", amdtpsCb.core.txState);
#endif
        return AMDTP_STATUS_BUSY;
    }

    //
    // Check if data length is valid
    //
    if ( len > AMDTP_MAX_PAYLOAD_SIZE )
    {
#ifdef AMDTP_DEBUG_ON
        debug_printf("data sending failed, exceed maximum payload, len = %d.", len);
#endif
        return AMDTP_STATUS_INVALID_PKT_LENGTH;
    }

    AmdtpBuildPkt(&amdtpsCb.core, type, encrypted, enableACK, buf, len);

    // send packet
    AmdtpSendPacketHandler(&amdtpsCb.core);

    return AMDTP_STATUS_SUCCESS;
}
