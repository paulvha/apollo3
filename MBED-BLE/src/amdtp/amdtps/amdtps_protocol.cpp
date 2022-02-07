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
// version 1.0 / February 2022 / Paulvh
//
//*****************************************************************************

/**
 * some background.
 *
 * The AMD transfer Protocol (AMDTP) allows sending larger datapackets between
 * client/central and server/peripheral. The default is set to max 512 bytes, which
 * will do for most sensor value exchange.
 *
 * The larger datapacket is broken down in smaller packages based on the agreed MTU size
 * by default the MTU size starts at 23 bytes. After removing the 3 bytes overhead it means
 * 20 bytes per sendingpacket (unless larger MTU size was agreed)
 *
 * On the receiving side the small chunks are combined again into a single large datapacket,
 * including an CRC-32 control to make sure we have the correct data received.
 *
 * The sending from server/peripheral is happening on a notify characteristic. We can send
 * very quickly on that, but the client/central side it needs to be able to keep up with
 * that speed with enough buffers. That is often NOT the case if you send 26 packet ( 512 /20)
 * very fast.
 *
 * Hence I have implemented that after each 20 byte (or MTU size) packet has been sent the
 * sending side (either client/central or server/peripheral) will wait for the other side to
 * confirm it has received this packet. Only after that it will send the next chunk / packet.
 *
 * The result is that all the packets are received, BUT each packet will take a turn-around time
 * of 375ms. Measured between Artemis Edge and Artemis ATP board.
 * Thus sending 512 bytes will take 10s on an a default MTU size.
 *
 * When connecting to BLEAK (on Ubuntu) the 512 bytes takes 3 seconds. MUCH faster, but still
 * 3000 / 512 = 6ms per byte = 1000 / 6 = 160 bytes per second.
 *
 * Not fast... but for many sensor this speed  will be acceptable as most produce only a small
 * amount of bytes
 *
 */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "amdtps_protocol.h"
#include "../crc32.h"

#if (defined AMDTPS_Debug) || (defined AMDTPS_SHOW_DATA)
#warning defined debug
void debug_float_s (float f);
void debug_print_s(const char* f, const char* F, uint16_t L);
void debug_printf_s(const char * fmt, ...);
#endif

/* Control block */
static struct
{
    bool                txReady;            // TRUE if ready to send notifications
    amdtpCb_t           core;               // defined in amdtp_common.h
}
amdtpsCb;

/**
 * @brief constructor and initialize variables
 */
AMDTPS::AMDTPS(BLE &ble):
    _AmdtpService(ble)
{
}

//*****************************************************************************
//
//! @brief initialize amdtp service
//!
//!
//! @return None
//
//*****************************************************************************
void
AMDTPS::amdtps_init()
{
    #ifdef AMDTPS_Debug
        debug_print_s(__func__, __FILE__, __LINE__);
    #endif

    memset(&amdtpsCb, 0, sizeof(amdtpsCb));

    amdtpsCb.core.lastRxPktSn = 0;                  // keep track of serial number 0 .. 15
    amdtpsCb.core.txPktSn = 0;

    resetPkt(&amdtpsCb.core.rxPkt);                 // set buffers
    amdtpsCb.core.rxPkt.data = rxPktBuf;

    resetPkt(&amdtpsCb.core.txPkt);
    amdtpsCb.core.txPkt.data = txPktBuf;

    resetPkt(&amdtpsCb.core.ackPkt);
    amdtpsCb.core.ackPkt.data = ackPktBuf;

    amdtpsCb.core.attMtuSize = ATT_DEFAULT_MTU;     // needed to break a package is pieces

    amdtpsCb.core.txState = AMDTP_STATE_TX_IDLE;    // enable sending
    amdtpsCb.txReady = true;
}

//*****************************************************************************
//! @brief reset package
//*****************************************************************************
void
AMDTPS::resetPkt(amdtpPacket_t *pkt)
{
    pkt->offset = 0;        // no data in buffer to start from
    pkt->header.pktType = AMDTP_PKT_TYPE_UNKNOWN; // unknown what is left
    pkt->len = 0;           // no length in buffer
}

//*****************************************************************************
//! Set callback to user program when data is ready to be returned.
//!
//! @param[in] cb The callback object that will be called when received data is ready
//*****************************************************************************
void AMDTPS::on_data_received(mbed::Callback<void(uint8_t *data, uint16_t len)> cb)
{
   _on_data_cb = cb;
}

//*****************************************************************************
//! @brief check for type of packet (Data or ACK) added version 3.1
//! return true for data packet
//*****************************************************************************
bool AMDTPS::Check_type(uint16_t len, uint8_t *pValue)
{
  uint16_t header;

  BYTES_TO_UINT16(header, &pValue[2]);              // second 2 bytes is header
  if((header & PACKET_TYPE_BIT_MASK) >> PACKET_TYPE_BIT_OFFSET == AMDTP_PKT_TYPE_DATA)
    return true;

  return false;
}

//*****************************************************************************
//! parse a received message
//!
//! Makes sure to receive a complete correct message
//!
//! AMDTP_STATUS_INVALID_PKT_LENGTH = Not enough data to start extracting the message
//! AMDTP_STATUS_INSUFFICIENT_BUFFER = not enough space to store the received message
//! AMDTP_STATUS_CRC_ERROR = CRC is not correct
//! AMDTP_STATUS_RECEIVE_DONE = message is complete and correct. Ready to go
//! AMDTP_STATUS_RECEIVE_CONTINUE = need more data packages. we are not complete yet.
//!
//!
//*****************************************************************************
eAmdtpStatus_t
AMDTPS::AmdtpReceivePkt(uint8_t handle, uint16_t len, uint8_t *pValue)
{
  amdtpPacket_t *pkt;

  uint8_t dataIdx = 0;
  uint32_t calDataCrc = 0;
  uint16_t header = 0;

  static bool FirstPacket = true;
  static bool Ptype;

#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif

  // do NOT use the handle to determine whether this is RX or ACK
  // the program has changed to sent ACK now also over RX, given that
  // some stacks are repeating whatever is received over notify.
  // When a NEW package is expected, check for the right package first
  // and remember that in a static bool.

  if (FirstPacket){
    Ptype = Check_type(len, pValue);
    FirstPacket = false;
  }

  if (Ptype) pkt = &amdtpsCb.core.rxPkt;
  else pkt = &amdtpsCb.core.ackPkt;

  // len must include length (2) and header (2)
  if (pkt->offset == 0 && len < AMDTP_PREFIX_SIZE_IN_PKT)
  {
#ifdef AMDTPS_Debug
    debug_printf_s("\rInvalid packet size !!!\n");
#endif
    FirstPacket = true;
    AmdtpSendReply(AMDTP_STATUS_INVALID_PKT_LENGTH, NULL, 0);
    return AMDTP_STATUS_INVALID_PKT_LENGTH;
  }

  // if new packet (not received packet(s) before, in case full message needs multiple packets
  // to be sent

  if (pkt->offset == 0)
  {
    NewChunkReceived = 0;                             // reset data chunk counter
    BYTES_TO_UINT16(pkt->len, pValue);                // first 2 bytes is length
    BYTES_TO_UINT16(header, &pValue[2]);              // second 2 bytes is header
    pkt->header.pktType = (header & PACKET_TYPE_BIT_MASK) >> PACKET_TYPE_BIT_OFFSET; // mask = 0xf, offset is 12 : top 4 bits
    pkt->header.pktSn = (header & PACKET_SN_BIT_MASK) >> PACKET_SN_BIT_OFFSET;       // mask = 0xf , ofsset is 8 : next for 4 bits
    pkt->header.encrypted = (header & PACKET_ENCRYPTION_BIT_MASK) >> PACKET_ENCRYPTION_BIT_OFFSET;  // mask = 1, offset is 7, next bit
    pkt->header.ackEnabled = (header & PACKET_ACK_BIT_MASK) >> PACKET_ACK_BIT_OFFSET; // mask 1, offset is 6, next bit
    dataIdx = AMDTP_PREFIX_SIZE_IN_PKT;     // 4

#ifdef AMDTPS_Debug
    debug_printf_s("\rpkt len = 0x%x, pkt header = 0x%x :\n", pkt->len, header);
    switch (pkt->header.pktType) {
      case  AMDTP_PKT_TYPE_DATA:
        debug_printf_s("\rtype = %d : DATA ", pkt->header.pktType);
        break;
      case  AMDTP_PKT_TYPE_ACK:
        debug_printf_s("\rtype = %d : ACK ", pkt->header.pktType);
        break;
      case  AMDTP_PKT_TYPE_CONTROL:
        debug_printf_s("\rtype = %d : Control ", pkt->header.pktType);
        break;
      default:
        debug_printf_s("\rtype = %d : Unknown ", pkt->header.pktType);
        break;
    }
    debug_printf_s("sn = %d, enc = %d, ackEnabled = %d\n", pkt->header.pktSn, pkt->header.encrypted, pkt->header.ackEnabled);
#endif
  }

  // make sure we have enough space for new data
  if (pkt->offset + len - dataIdx > AMDTP_PACKET_SIZE)   // make sure it fits in 512 + 4 +4 (max length defined amdtp_common.h:  512)
  {
#ifdef AMDTPS_Debug
    debug_printf_s("\rNot enough buffer size!!!\n");
#endif

    resetPkt(pkt);
    FirstPacket = true;
    AmdtpSendReply(AMDTP_STATUS_INSUFFICIENT_BUFFER, NULL, 0);
    return AMDTP_STATUS_INSUFFICIENT_BUFFER;
  }

  // copy new data into buffer and also save crc into it if it's the last packet in a message
  // 4 bytes crc is included in pkt length

  memcpy(pkt->data + pkt->offset, pValue + dataIdx, len - dataIdx); // skip the header
  pkt->offset += (len - dataIdx);

  // whole packet received
  if (pkt->offset >= pkt->len)
  {
    // next time again a new packet of a new message
    FirstPacket = true;

    uint32_t peerCrc = 0;
    //
    // check CRC
    //
    BYTES_TO_UINT32(peerCrc, pkt->data + pkt->len - AMDTP_CRC_SIZE_IN_PKT);
    calDataCrc = CalcCrc32(0xFFFFFFFFU, pkt->len - AMDTP_CRC_SIZE_IN_PKT, pkt->data);

    if (peerCrc != calDataCrc)
    {
#ifdef AMDTPS_Debug
      debug_printf_s("\rcrc error :\ncalDataCrc = 0x%x ", calDataCrc);
      debug_printf_s("peerCrc = 0x%x\n", peerCrc);
#endif

      // reset pkt
      resetPkt(pkt);

      AmdtpSendReply(AMDTP_STATUS_CRC_ERROR, NULL, 0);
      return AMDTP_STATUS_CRC_ERROR;
    }

    // we have it all, now handle request
    AmdtpPacketHandler((eAmdtpPktType_t)pkt->header.pktType, pkt->len - AMDTP_CRC_SIZE_IN_PKT, pkt->data);

    return AMDTP_STATUS_RECEIVE_DONE;
  }
  // if requested to confirm packet received (not on last packet)
  if (pkt->header.pktType == AMDTP_PKT_TYPE_DATA && pkt->header.ackEnabled){

    // count this new packet (the packet count starts with 1)
    NewChunkReceived++;
    AmdtpSendControl(AMDTP_CONTROL_SEND_READY, &NewChunkReceived,1);
  }
  // not all had been received
  return AMDTP_STATUS_RECEIVE_CONTINUE;
}

//*****************************************************************************
//
// AMDTP packet handler
// what to do now a complete package has been RECEIVED ?
//
//*****************************************************************************
void
AMDTPS::AmdtpPacketHandler(eAmdtpPktType_t type, uint16_t len, uint8_t *buf)
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif

  switch(type)
  {
    case AMDTP_PKT_TYPE_DATA:
      //
      // data package received
      //
      // record packet serial number

      amdtpsCb.core.lastRxPktSn = amdtpsCb.core.rxPkt.header.pktSn;

      AmdtpSendReply(AMDTP_STATUS_SUCCESS, NULL, 0);

      // finally we are going to user level to handle the request
      if (_on_data_cb)
        _on_data_cb(buf, len);

#ifdef AMDTPS_SHOW_DATA
      else {
            debug_printf_s("\r\n=========== received data (no callback was set) =====================\r\n");

            for (uint16_t i =0; i < len; i++) debug_printf_s("0x%02X ", buf[i]);
            debug_printf_s("\r\n");
      }
#endif
      resetPkt(&amdtpsCb.core.rxPkt);
      break;

    case AMDTP_PKT_TYPE_ACK:
    {
      eAmdtpStatus_t status = (eAmdtpStatus_t)buf[0];

      if (amdtpsCb.core.txState != AMDTP_STATE_WAITING_ACK){

#ifdef AMDTPS_Debug
        debug_printf_s("Warning : Received an unexpected ACK\n");
#endif
      }

      amdtpsCb.core.txState = AMDTP_STATE_TX_IDLE;

      if (status == AMDTP_STATUS_CRC_ERROR || status == AMDTP_STATUS_RESEND_REPLY)
      {
        // resend packet
        AmdtpSendPacketHandler();
      }
      else
      {
        // increase packet serial number if send successfully
        if (status == AMDTP_STATUS_SUCCESS)
        {
          amdtpsCb.core.txPktSn++;

          if (amdtpsCb.core.txPktSn == 16)     // max 4 bits part of the header (so count 0 - 15)
          {
            amdtpsCb.core.txPktSn = 0;
          }
        }

        // reset send packet
        resetPkt(&amdtpsCb.core.txPkt);
      }

      resetPkt(&amdtpsCb.core.ackPkt);
    }
      break;

  case AMDTP_PKT_TYPE_CONTROL:
  {
    eAmdtpControl_t control = (eAmdtpControl_t)buf[0];
    uint8_t resendPktSn = buf[1];

    // we have more chunks of data to send so we requested
    // from the peripheral a confirmation that we can send next part
    if (control == AMDTP_CONTROL_SEND_READY)
    {
       NewChunkReceived = resendPktSn;
       AmdtpSendPacketHandler();
    }

    else if (control == AMDTP_CONTROL_RESEND_REQ)
    {

      resetPkt(&amdtpsCb.core.rxPkt);

      if (resendPktSn > amdtpsCb.core.lastRxPktSn)
      {
        AmdtpSendReply(AMDTP_STATUS_RESEND_REPLY, NULL, 0);
      }
      else if (resendPktSn == amdtpsCb.core.lastRxPktSn)
      {
        AmdtpSendReply(AMDTP_STATUS_SUCCESS, NULL, 0);
      }
#ifdef AMDTPS_Debug
      else
      {
         debug_printf_s("\rCan not act on Control package resendPktSn = %d, lastRxPktSn = %d\n", resendPktSn, amdtpsCb.core.lastRxPktSn);
      }
    }
    else
    {
      debug_printf_s("\rUnexpected contrl request  = %d\n", control);
#endif
    }

    resetPkt(&amdtpsCb.core.ackPkt);
  }
    break;

  default:
    break;
  }
}

//*****************************************************************************
//
// create packet to send
//
//*****************************************************************************
void
AMDTPS::AmdtpBuildPkt(eAmdtpPktType_t type, bool encrypted, bool enableACK, uint8_t *buf, uint16_t len)
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif
  uint16_t header = 0;
  uint32_t calDataCrc;
  amdtpPacket_t *pkt;

  if (type == AMDTP_PKT_TYPE_DATA)
  {
    pkt = &amdtpsCb.core.txPkt;
    header = amdtpsCb.core.txPktSn << PACKET_SN_BIT_OFFSET;  // add 4 bit serial number to right offset
  }
  else
  {
    pkt = &amdtpsCb.core.ackPkt;                             // acknowledgement of earlier received data
  }

  //
  // Prepare header frame to be sent first
  //
  // length
  pkt->len = len + AMDTP_PREFIX_SIZE_IN_PKT + AMDTP_CRC_SIZE_IN_PKT;  // len of data + header & len (4) + CRC (4 ytes)
  pkt->data[0]  = (len + AMDTP_CRC_SIZE_IN_PKT) & 0xff;               // LSB
  pkt->data[1]  = ((len + AMDTP_CRC_SIZE_IN_PKT) >> 8) & 0xff;        // MSB length

  // header
  header = header | (type << PACKET_TYPE_BIT_OFFSET);                 // add 4 bit type
  if (encrypted)
  {
    header = header | (1 << PACKET_ENCRYPTION_BIT_OFFSET);            // set encryption bit (not used really)
  }

  if (pkt->len > (amdtpsCb.core.attMtuSize - 3)) {
    header = header | (1 << PACKET_ACK_BIT_OFFSET);                   // set controlbit to send reply on receipt
  }

  pkt->data[2] = (header & 0xff);                                     // set header
  pkt->data[3] = (header >> 8);

  // copy data
  memcpy(&(pkt->data[AMDTP_PREFIX_SIZE_IN_PKT]), buf, len);           // now add the data
  calDataCrc = CalcCrc32(0xFFFFFFFFU, len, buf);                      // calculate the CRC

  // add checksum
  pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len] = (calDataCrc & 0xff);    // add the 32 bit CRC
  pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 1] = ((calDataCrc >> 8) & 0xff);
  pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 2] = ((calDataCrc >> 16) & 0xff);
  pkt->data[AMDTP_PREFIX_SIZE_IN_PKT + len + 3] = ((calDataCrc >> 24) & 0xff);

#ifdef AMDTPS_SHOW_DATA
  if (type == AMDTP_PKT_TYPE_DATA)
    debug_printf_s("\r\n=========== Data Prepared for sending ====================\r\n");
  else
    debug_printf_s("\r\n=========== Ack Prepared for sending =====================\r\n");

  for (uint16_t i =0; i < pkt->len; i++) debug_printf_s("0x%02X ", pkt->data[i]);
  debug_printf_s("\r\n");
#endif

}

//*****************************************************************************
//
// Send control message to Receiver
//
//*****************************************************************************
void
AMDTPS::AmdtpSendControl(eAmdtpControl_t control, uint8_t *data, uint16_t len)
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif

  uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN] = {0};

  buf[0] = control;

  // add any data
  if (len > 0)  memcpy(buf + 1, data, len);

  amdtpsSendAck(AMDTP_PKT_TYPE_CONTROL, false, false, buf, len + 1);
}

//*****************************************************************************
//
// Send Reply (ACK - type with status and optional data) to Sender
//
//*****************************************************************************
void
AMDTPS::AmdtpSendReply(eAmdtpStatus_t status, uint8_t *data, uint16_t len)
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif

  uint8_t buf[ATT_DEFAULT_PAYLOAD_LEN] = {0}; // max payload length for most PDU on bluetooth

  buf[0] = status;                    // first byte is status

  // add any data
  if (len > 0)  memcpy(buf + 1, data, len);

  amdtpsSendAck(AMDTP_PKT_TYPE_ACK, false, false, buf, len + 1);
}

//*****************************************************************************
//
// @brief  : send acknowledgement over the ACK handle
// @param type : ack or control
// @param encrypted : set encryption (not used anywhere.. but who knows in the future ??)
// @param enableACK : not used anywhere.. but who knows in the future ??
// @param buf : data to sent
// @param len : length of data to sent
//*****************************************************************************
void
AMDTPS::amdtpsSendAck(eAmdtpPktType_t type, bool encrypted, bool enableACK, uint8_t *buf, uint16_t len)
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif

  // create packet with CRC etc
  AmdtpBuildPkt(type, encrypted, enableACK, buf, len);

  _AmdtpService.ACK_update(amdtpsCb.core.ackPkt.data,amdtpsCb.core.ackPkt.len);
}

//*****************************************************************************
//
// Send data to client (called from Sketch)
// return :
// -1  error
//  0  sending of package has been completed
//  1  Pending to Send next chunk of data
//
//*****************************************************************************
int AMDTPS::AmdtpSendData(uint8_t *buf, uint16_t len)
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif
  //                                                      encrypted, enableACK
  eAmdtpStatus_t st = AmdtpsSendPacket(AMDTP_PKT_TYPE_DATA, false, false, buf, len);

  if(st != AMDTP_STATUS_SUCCESS){
       return -1;
  }

  if (SendingNOtComplete) return 1;

  return 0;
}

//*****************************************************************************
//
// Is sending chunk of data to client complete
//
// return :
// True : complete sending chunk
// false : not complete sending
//
//*****************************************************************************

bool AMDTPS::AmdtpSendComplete()
{
  return(! SendingNOtComplete);
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
//! AMDTP_STATUS_TX_NOT_READY;
//! AMDTP_STATUS_BUSY
//! AMDTP_STATUS_INVALID_PKT_LENGTH
//! AMDTP_STATUS_SUCCESS
//!
//*****************************************************************************
eAmdtpStatus_t
AMDTPS::AmdtpsSendPacket(eAmdtpPktType_t type, bool encrypted, bool enableACK, uint8_t *buf, uint16_t len)
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif

  //
  // Check if ready to send notification and if running as server
  //
  if (! amdtpsCb.txReady )
  {

#ifdef AMDTPS_Debug
    debug_printf_s("data sending failed, Not ready for notification.\n");
#endif
    return AMDTP_STATUS_TX_NOT_READY;
  }

  //
  // Check if the service is idle to send
  //
  if (amdtpsCb.core.txState != AMDTP_STATE_TX_IDLE)
  {

#ifdef AMDTPS_Debug
    debug_printf_s("\rData sending failed, tx state = %d\n", amdtpsCb.core.txState);
#endif
    return AMDTP_STATUS_BUSY;
  }

  //
  // Check if data length is valid
  //
  if ( len > AMDTP_MAX_PAYLOAD_SIZE )
  {
#ifdef AMDTPS_Debug
    debug_printf_s("\rData sending failed, exceed maximum payload, len = %d.\n", len);
#endif
    return AMDTP_STATUS_INVALID_PKT_LENGTH;
  }

  // build packet to sent
  AmdtpBuildPkt(type, encrypted, enableACK, buf, len);

  // send packet
  AmdtpSendPacketHandler();

  return AMDTP_STATUS_SUCCESS;
}

//*****************************************************************************
//
// sent data package. This will add the status around checks and break down
// in chuncks
//
//*****************************************************************************
void
AMDTPS::AmdtpSendPacketHandler()
{
#ifdef AMDTPS_Debug
  debug_print_s(__func__, __FILE__, __LINE__);
#endif

  uint16_t transferSize;
  uint16_t remainingBytes;
  static uint8_t PacketCount;          // counter for chunks of data

  amdtpPacket_t *txPkt = &amdtpsCb.core.txPkt;

  if (amdtpsCb.core.txState == AMDTP_STATE_TX_IDLE ) {
    txPkt->offset = 0;
    amdtpsCb.core.txState = AMDTP_STATE_SENDING;
    PacketCount = 0;
  }

  if (amdtpsCb.core.txState == AMDTP_STATE_SENDING ) {

#ifdef AMDTPC_Debug
    if (SendingNOtComplete) {
        if (PacketCount != NewChunkReceived)
            printf("Warning: packages out of sequence. Got %d, expected %d.\n", NewChunkReceived, PacketCount);
    }
#endif

    // send small pieces of the packet. It just restricts sending to mtusize -3
    // assumed mtusize is set during amdtp_init()
    if (txPkt->offset < txPkt->len)
    {
        remainingBytes = txPkt->len - txPkt->offset;
        transferSize = ((amdtpsCb.core.attMtuSize - 3) > remainingBytes)
                                            ? remainingBytes
                                            : (amdtpsCb.core.attMtuSize - 3);

        _AmdtpService.TX_update(&txPkt->data[txPkt->offset], transferSize);

        txPkt->offset += transferSize;

        // check whether we are done
        if(txPkt->offset >= txPkt->len){

            // done sending packet
            amdtpsCb.core.txState = AMDTP_STATE_WAITING_ACK;
            SendingNOtComplete = false;
            return;
        }
        else {

            // update packet counter (start with 1)
            PacketCount++;

            // indicate we are NOT ready with sending
            SendingNOtComplete = true;
        }
    }
  }
}

#if (defined AMDTPS_Debug) || (defined AMDTPS_SHOW_DATA)
    // ****************************************
    //
    // Debug print functions
    //
    // ****************************************

    #define DEBUG_UART_BUF_LEN 256

    char debug_buffer_s [DEBUG_UART_BUF_LEN];

    void debug_float_s (float f) {
        SERIAL_PORT.print(f);
    }

    void debug_print_s(const char* f, const char* F, uint16_t L){
        SERIAL_PORT.print("\rfm: ");
        SERIAL_PORT.print(f);
        SERIAL_PORT.print(", file: ");
        SERIAL_PORT.print(F);
        SERIAL_PORT.print(", line: ");
        SERIAL_PORT.println(L);
    }

    void debug_printf_s(const char * fmt, ...){
        va_list args;
        va_start (args, fmt);
        vsnprintf(debug_buffer_s, DEBUG_UART_BUF_LEN, (const char*)fmt, args);
        va_end (args);

        // some boards can only handle up to 64 characters at a time
        uint8_t i = 0;
        uint8_t len = strlen(debug_buffer_s);

        for(i = 0; i < len ;i++) {
          SERIAL_PORT.print((char) debug_buffer_s[i]);
        }
    }
#endif // (defined AMDTPS_Debug) || (defined AMDTPS_SHOW_DATA)
