// ****************************************************************************
//
//  amdtpc_main.c
//! @file
//!
//! @brief Ambiq Micro AMDTP client.
//!
//! @{
//
// ****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2020, Ambiq Micro
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
// This is part of revision 2.4.2 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include <string.h>
#include <stdbool.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "bstream.h"
#include "app_api.h"
#include "amdtpc_api.h"
#include "svc_amdtp.h"
#include "wsf_trace.h"

//extra includes
#include "ble_menu.h"


// enable debug for this file
// AM_DEBUG_PRINTF is defined in ble_menu.h
#ifdef AM_DEBUG_PRINTF
#define AMDTPC_MAIN_DEBUG_ON 
#endif

static void amdtpcHandleWriteResponse(attEvt_t *pMsg);

//*****************************************************************************
//
// Global variables
//
//*****************************************************************************

uint8_t rxPktBuf[AMDTP_PACKET_SIZE];
uint8_t txPktBuf[AMDTP_PACKET_SIZE];
uint8_t ackPktBuf[20];


/**************************************************************************************************
  Local Variables
**************************************************************************************************/

/* UUIDs */
static const uint8_t amdtpSvcUuid[] = {ATT_UUID_AMDTP_SERVICE};    /*! AMDTP service */
static const uint8_t amdtpRxChUuid[] = {ATT_UUID_AMDTP_RX};        /*! AMDTP Rx */
static const uint8_t amdtpTxChUuid[] = {ATT_UUID_AMDTP_TX};        /*! AMDTP Tx */
static const uint8_t amdtpAckChUuid[] = {ATT_UUID_AMDTP_ACK};      /*! AMDTP Ack */

/* Characteristics for discovery */

/*! Proprietary data */
static const attcDiscChar_t amdtpRx =
{
  amdtpRxChUuid,
  ATTC_SET_REQUIRED | ATTC_SET_UUID_128
};

static const attcDiscChar_t amdtpTx =
{
  amdtpTxChUuid,
  ATTC_SET_REQUIRED | ATTC_SET_UUID_128
};

/*! AMDTP Tx CCC descriptor */
static const attcDiscChar_t amdtpTxCcc =
{
  attCliChCfgUuid,
  ATTC_SET_REQUIRED | ATTC_SET_DESCRIPTOR
};

static const attcDiscChar_t amdtpAck =
{
  amdtpAckChUuid,
  ATTC_SET_REQUIRED | ATTC_SET_UUID_128
};

/*! AMDTP Tx CCC descriptor */
static const attcDiscChar_t amdtpAckCcc =
{
  attCliChCfgUuid,
  ATTC_SET_REQUIRED | ATTC_SET_DESCRIPTOR
};

/*! List of characteristics to be discovered; order matches handle index enumeration  */
static const attcDiscChar_t *amdtpDiscCharList[] =
{
  &amdtpRx,                  /*! Rx */
  &amdtpTx,                  /*! Tx */
  &amdtpTxCcc,               /*! Tx CCC descriptor */
  &amdtpAck,                 /*! Ack */
  &amdtpAckCcc               /*! Ack CCC descriptor */
};

/* sanity check:  make sure handle list length matches characteristic list length */
WSF_ASSERT(AMDTP_HDL_LIST_LEN == ((sizeof(amdtpDiscCharList) / sizeof(attcDiscChar_t *))));


/* Control block */
static struct
{
    bool_t                  txReady;                // TRUE if ready to send notifications
    uint16_t                attRxHdl;               // RX for server, so TX for client
    uint16_t                attTxHdl;               // TX for client, so RX for server
    uint16_t                attAckHdl;              // ACK for server and client
    amdtpCb_t               core;
}
amdtpcCb;

/*************************************************************************************************/
/*!
 *  \fn     AmdtpDiscover
 *
 *  \brief  Perform service and characteristic discovery for AMDTP service.
 *          Parameter pHdlList must point to an array of length AMDTP_HDL_LIST_LEN.
 *          If discovery is successful the handles of discovered characteristics and
 *          descriptors will be set in pHdlList.
 *
 *  \param  connId    Connection identifier.
 *  \param  pHdlList  Characteristic handle list.
 *
 *  \return None.
 */
/*************************************************************************************************/
void
AmdtpcDiscover(dmConnId_t connId, uint16_t *pHdlList)
{
#ifdef AMDTPC_MAIN_DEBUG_ON 
  am_menu_printf("%s handle %x, Connection opened / discover services starting\r\n",__func__, pHdlList);
#endif

  AppDiscFindService(connId, ATT_128_UUID_LEN, (uint8_t *) amdtpSvcUuid,
                     AMDTP_HDL_LIST_LEN, (attcDiscChar_t **) amdtpDiscCharList, pHdlList);
}

//*****************************************************************************
//
// Send data to Server
//
//*****************************************************************************
static void
amdtpcSendData(uint8_t *buf, uint16_t len)
{
    dmConnId_t connId;
    if ((connId = AppConnIsOpen()) == DM_CONN_ID_NONE)
    {

#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s DM_CONN_ID_NONE", __func__);
#endif
        return;
    }

    if (amdtpcCb.attTxHdl != ATT_HANDLE_NONE)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s: attTxHdl = 0x%x ...\r\n",__func__, amdtpcCb.attTxHdl);
#endif
      AttcWriteCmd(connId, amdtpcCb.attTxHdl, len, buf);
      amdtpcCb.txReady = false;
    }
    else
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s: INVALID HANDLE \r\n",__func__);
#endif
    }
}

static eAmdtpStatus_t
amdtpcSendAck(eAmdtpPktType_t type, bool_t encrypted, bool_t enableACK, uint8_t *buf, uint16_t len)
{
    dmConnId_t connId;

    AmdtpBuildPkt(&amdtpcCb.core, type, encrypted, enableACK, buf, len);

    if ((connId = AppConnIsOpen()) == DM_CONN_ID_NONE)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s no connection\r\n",__func__);
#endif
        return AMDTP_STATUS_TX_NOT_READY;
    }

    if (amdtpcCb.attAckHdl != ATT_HANDLE_NONE)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s Send Ack Data = %d\r\n", __func__, amdtpcCb.core.ackPkt.data);
#endif
      AttcWriteCmd(connId, amdtpcCb.attAckHdl, amdtpcCb.core.ackPkt.len, amdtpcCb.core.ackPkt.data);
    }
    else
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s Invalid attAckHdl = 0x%x\r\n",__func__, amdtpcCb.attAckHdl);
#endif
      return AMDTP_STATUS_TX_NOT_READY;
    }
    
    return AMDTP_STATUS_SUCCESS;
}

void
amdtpc_init(wsfHandlerId_t handlerId, amdtpRecvCback_t recvCback, amdtpTransCback_t transCback)
{
    memset(&amdtpcCb, 0, sizeof(amdtpcCb));
    amdtpcCb.txReady = false;
    amdtpcCb.core.txState = AMDTP_STATE_TX_IDLE;
    amdtpcCb.core.rxState = AMDTP_STATE_INIT;
    amdtpcCb.core.timeoutTimer.handlerId = handlerId;

    amdtpcCb.core.lastRxPktSn = 0;
    amdtpcCb.core.txPktSn = 0;

    resetPkt(&amdtpcCb.core.rxPkt);
    amdtpcCb.core.rxPkt.data = rxPktBuf;

    resetPkt(&amdtpcCb.core.txPkt);
    amdtpcCb.core.txPkt.data = txPktBuf;

    resetPkt(&amdtpcCb.core.ackPkt);
    amdtpcCb.core.ackPkt.data = ackPktBuf;

    amdtpcCb.core.recvCback = recvCback;
    amdtpcCb.core.transCback = transCback;

    amdtpcCb.core.txTimeoutMs = TX_TIMEOUT_DEFAULT;

    amdtpcCb.core.data_sender_func = amdtpcSendData;
    amdtpcCb.core.ack_sender_func = amdtpcSendAck;
}

void
amdtpc_conn_close(dmEvt_t *pMsg)
{
    /* clear connection */
    WsfTimerStop(&amdtpcCb.core.timeoutTimer);
    amdtpcCb.txReady = false;
    amdtpcCb.core.txState = AMDTP_STATE_TX_IDLE;
    amdtpcCb.core.rxState = AMDTP_STATE_INIT;
    amdtpcCb.core.lastRxPktSn = 0;
    amdtpcCb.core.txPktSn = 0;
    resetPkt(&amdtpcCb.core.rxPkt);
    resetPkt(&amdtpcCb.core.txPkt);
    resetPkt(&amdtpcCb.core.ackPkt);
am_menu_printf("amdtpc_conn_close() no connection\r\n");
am_menu_printf("amdtpc_conn_close() no connection\r\n");
am_menu_printf("amdtpc_conn_close() no connection\r\n");
am_menu_printf("amdtpc_conn_close() no connection\r\n");
am_menu_printf("amdtpc_conn_close() no connection\r\n");
am_menu_printf("amdtpc_conn_close() no connection\r\n");
#ifdef BLE_MENU
    // let menu structure know
    menuRxDataLen = 0;
    add_menu_input(NOT_CONN_CLOSED);
    add_menu_input('\r');
#endif

}

//*****************************************************************************
//
// initialise handles
// txHdl  TX for client = RX for server
// rxHdl  RX for client = TX for server
// ackHdl acknowledgement handle
//
//*****************************************************************************
void
amdtpc_start(uint16_t txHdl, uint16_t rxHdl, uint16_t ackHdl, uint8_t timerEvt)
{
    amdtpcCb.txReady = true;
    amdtpcCb.attRxHdl = rxHdl;
    amdtpcCb.attTxHdl = txHdl;
    amdtpcCb.attAckHdl = ackHdl;
    amdtpcCb.core.timeoutTimer.msg.event = timerEvt;

    dmConnId_t connId;

    if ((connId = AppConnIsOpen()) == DM_CONN_ID_NONE)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
       am_menu_printf("amdtpc_start() no connection\r\n");
       return;
#endif
    }

    amdtpcCb.core.attMtuSize = AttGetMtu(connId);
#ifdef AMDTPC_MAIN_DEBUG_ON 
    am_menu_printf("MTU size = %d bytes\r\n", amdtpcCb.core.attMtuSize);
#endif

#ifdef BLE_MENU
    // let menu structure know
    menuRxDataLen = 0;
    add_menu_input(NOT_CONN_OPENED);
    add_menu_input('\r');
#endif
}

//*****************************************************************************
//
// Timer Expiration handler
//
//*****************************************************************************
void
amdtpc_timeout_timer_expired(wsfMsgHdr_t *pMsg)
{
    uint8_t data[1];
    data[0] = amdtpcCb.core.txPktSn;

#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s amdtpc tx timeout, txPktSn = %d\r\n",__func__, amdtpcCb.core.txPktSn);
#endif

    AmdtpSendControl(&amdtpcCb.core, AMDTP_CONTROL_RESEND_REQ, data, 1);
    
    // fire a timer for receiving an AMDTP_STATUS_RESEND_REPLY ACK
    WsfTimerStartMs(&amdtpcCb.core.timeoutTimer, amdtpcCb.core.txTimeoutMs);
}

/*************************************************************************************************/
/*!
 *  \fn     amdtpcValueNtf
 *
 *  \brief  Process a received ATT notification.
 *
 *  \param  pMsg    Pointer to ATT callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static uint8_t
amdtpcValueNtf(attEvt_t *pMsg)
{
  eAmdtpStatus_t status = AMDTP_STATUS_UNKNOWN_ERROR;
  amdtpPacket_t *pkt = NULL;
  
#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s Received Notification from amdtp_proc_msg \r\n",__func__);
      am_menu_printf(" handle = 0x%x\r\n, pMsg->value = ", pMsg->handle);
      for (int i = 0; i < pMsg->valueLen; i++)
      {
          am_menu_printf("%02X ", pMsg->pValue[i]);
      }
#endif

  if (pMsg->handle == amdtpcCb.attRxHdl)
  {
      status = AmdtpReceivePkt(&amdtpcCb.core, &amdtpcCb.core.rxPkt, pMsg->valueLen, pMsg->pValue);
  }
  else if ( pMsg->handle == amdtpcCb.attAckHdl)
  {
      status = AmdtpReceivePkt(&amdtpcCb.core, &amdtpcCb.core.ackPkt, pMsg->valueLen, pMsg->pValue);
  }

  if (status == AMDTP_STATUS_RECEIVE_DONE)
  {
      if (pMsg->handle == amdtpcCb.attRxHdl)
      {
          pkt = &amdtpcCb.core.rxPkt;
      }
      else if (pMsg->handle == amdtpcCb.attAckHdl)
      {
          pkt = &amdtpcCb.core.ackPkt;
      }

      AmdtpPacketHandler(&amdtpcCb.core, (eAmdtpPktType_t)pkt->header.pktType, pkt->len - AMDTP_CRC_SIZE_IN_PKT, pkt->data);
  }

  return ATT_SUCCESS;
}

// write response received...
static void
amdtpcHandleWriteResponse(attEvt_t *pMsg)
{
#ifdef AMDTPC_MAIN_DEBUG_ON 
  am_menu_printf("%s SENDING DATA TO SERVER: \r\n",__func__);
  am_menu_printf("handle = 0x%x, pMsgLength = %d\r\n pMsg->pValue = ", pMsg->handle, pMsg->valueLen); 
  for (int i = 0; i < pMsg->valueLen; i++)
  {
      am_menu_printf(" pMsg->value[%d] = %02x\r\n ", i, pMsg->pValue[i]);
  }
  am_menu_printf("\r\n");
#endif

  if (pMsg->hdr.status == ATT_SUCCESS && pMsg->handle == amdtpcCb.attTxHdl)
  {
    amdtpcCb.txReady = true;
    
    // process next data
    AmdtpSendPacketHandler(&amdtpcCb.core);
  }
}

void
amdtpc_proc_msg(wsfMsgHdr_t *pMsg)
{
#ifdef AMDTPC_MAIN_DEBUG_ON 
    am_menu_printf("%s: event = %i ",__func__, pMsg->event);
#endif
    if (pMsg->event == DM_CONN_OPEN_IND)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf(" DM_CONN_OPEN_IND \r\n");
#endif
    }
    else if (pMsg->event == DM_CONN_CLOSE_IND)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf(" DM_CONN_CLOSE_IND \r\n");
#endif
        amdtpc_conn_close((dmEvt_t *) pMsg);
    }
    else if (pMsg->event == DM_CONN_UPDATE_IND)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf(" DM_CONN_UPDATE_IND \r\n");
#endif
    }
    else if (pMsg->event == amdtpcCb.core.timeoutTimer.msg.event)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf(" Timer expired \r\n");
#endif
       amdtpc_timeout_timer_expired(pMsg);
    }
    else if (pMsg->event == ATTC_WRITE_CMD_RSP)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf(" ATTC_WRITE_CMD_RSP\r\n");
#endif
        amdtpcHandleWriteResponse((attEvt_t *) pMsg);
    }
    else if (pMsg->event == ATTC_HANDLE_VALUE_NTF)
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf(" ATTC_HANDLE_VALUE_NTF\r\n");
#endif
        amdtpcValueNtf((attEvt_t *) pMsg);
    }
}

//*****************************************************************************
//
//! @brief Send data to Server via write command
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
AmdtpcSendPacket(eAmdtpPktType_t type, bool_t encrypted, bool_t enableACK, uint8_t *buf, uint16_t len)
{
  
#ifdef AMDTPC_MAIN_DEBUG_ON 
    am_menu_printf("%s- EXECUTING SEND PACKET, ",__func__);
    am_menu_printf("Data Length =  %d, sending :", len);
    for ( uint16_t i = 0 ; i < len ; i++ ){
      am_menu_printf("0x%X ", buf[i]);
    }
    am_menu_printf("\r\n");
#endif
   
    // Check if the service is idle to send
    //
    if ( amdtpcCb.core.txState != AMDTP_STATE_TX_IDLE )
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
      am_menu_printf("%s data sending blocked, tx state = %d\r\n",__func__, amdtpcCb.core.txState);
#endif
      return AMDTP_STATUS_BUSY;
    }

    // Check if data length is valid
    //
    if ( len > AMDTP_MAX_PAYLOAD_SIZE )
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf("%s data sending failed, exceed maximum payload, len = %d\r\n", __func__, len);
#endif
        return AMDTP_STATUS_INVALID_PKT_LENGTH;
    }

    //
    // Check if ready to send notification
    //

    if ( !amdtpcCb.txReady ) //set in callback amdtpsHandleValueCnf
    {
#ifdef AMDTPC_MAIN_DEBUG_ON 
        am_menu_printf("%s - data sending failed, not ready for notification.\r\n",__func__);
#endif
        return AMDTP_STATUS_TX_NOT_READY;
    }

    AmdtpBuildPkt(&amdtpcCb.core, type, encrypted, enableACK, buf, len);

    // send packet
    AmdtpSendPacketHandler(&amdtpcCb.core);

    return AMDTP_STATUS_SUCCESS;
}
