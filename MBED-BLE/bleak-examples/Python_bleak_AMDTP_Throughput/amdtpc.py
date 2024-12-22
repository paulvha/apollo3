#!/usr/bin/python3
# -*- coding: UTF-8 -*-
from crc import crc
import time

"""
Paul van Haastrecht - February 2022 / version 1.0

Example of cross platform data transmission between Artemis CORDIO AMDTP peripheral/server and Python using the Bleak Project

The AMD Transfer Protocol enables a exchanging larg datapackages between Peripheral and central. It does this by breaking that data packet into smaller packages. The size is based on the agreed MTU size. Once all the packages have been received it will perform a CRC checked on the total received package.

The client / central is trying to connect to an BLE peripheral / server based on the device address. Once connected it will subscribe
to the notify characteristic to receive data, as send by the Artemis / Apollo3 sketch.

# Receiving
The data received on the notify characteristic is passed on to this AMTDP service with a call to AmdtpReceivePkt(). Here it will check and combine different packages into a larger data packet which is passed on to the AmdtpPackHandler(). If the packet is data, a call back to the user program will be done to hand off.
If the packet is an  acknowledgement on earlier sent data by the client it will be handled internally. If the packet is a control packet, it might be a confirmation on a certain serial number.
If there is an error in the received packet an error message will be send.
Each received data-package (NOT ACk, NOT control) is acknowledged by calling AmdtpSendReply(), which will call AmdtpcSendAck(). The ACK packet is build with AmdtpBuildPkt() and then send to the peripheral send characteristic.

# Sending
Any data to be send to the server is handled by calling AmdtpSendData(). Parameters are added and AmdtpcSendPacket() is called before calling AmdtpBuildPkt(). Here a headervalues and CRC is added, now  AmdtpSendPacketHandler() is called to every time send a chunk of data that will fit the MTU size.

For this python side, the Bluetooth Low Energy platform Agnostic Klient for Python (Bleak) project is used for Cross Platform Support.
It has been tested with Ubuntu 20.04

 **************************************************************************************************
 * some background.
 *
 * The AMD transfer Protocol (AMDTP) allows sending larger datapackets between
 * client/central and server/peripheral. The default is set to max 512 bytes, which
 * will do for most sensor value exchange.
 *
 * The larger datapacket is broken down in smaller packages based on the agreed MTU size.
 * By default the MTU size starts at 23 bytes. After removing the 3 bytes overhead it means
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
 * Hence it is implemented that after each 20 byte (or MTU size) packet has been sent the
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
 *
 *****************************************************************************************/


**************** example sending **********************
client request 7

calling AmtpdSendData()

=========== Data Prepared for sending ====================
22:07:12.269 -> 0x5 0x0 0x0 0x10 0x7 0x2E 0x7A 0x66 0x4C

0x5 0x0 = 0x0 0x5 length of TOTAL data packet

0x0 0x10 = 0x10 x00 header
0x001 0x0000 0x0000 0x0000

0001 0000 0000 0000
||||                   ptype (1 = data)
     ||||              serial number (0)
          |            encrypted (NO, not used now)
           |           ACK enabled (NO, not used now)
            || ||||    reserved

0x7                    Data (request was send)
0x2E 0x7A 0x66 0x4C    CRC32


************* example ACK *********************************
When a data packet has been received for the peripheral

=========== Ack Prepared for sending =====================
22:09:02.305 -> 0x5 0x0 0x0 0x20 0x0 0x8D 0xEF 0x2 0xD2

0x5 0x0 = 0x0 0x5 length of TOTAL data packet

0x0 0x20 = 0x20 x00 header
0x0010 0x0000 0x0000 0x0000

0010 0000 0000 0000
||||                   ptype (2 = ACK)
     ||||              serial number (0)
          |            encrypted (NO, not used now)
           |           ACK enabled (NO, not used now)
            || ||||    reserved

0x0                    AMDTP_STATUS_SUCCESS
0x8D 0xEF 0x2 0xD2     CRC32

"""
#********************************************************************
ATT_DEFAULT_MTU   = 23
ATT_MAX_MTU       = 200                     # Maximum value of ATT_MTU

# eAmdtpStatus
AMDTP_STATUS_SUCCESS =               0x00
AMDTP_STATUS_CRC_ERROR =             0x01
AMDTP_STATUS_INVALID_METADATA_INFO = 0x02
AMDTP_STATUS_INVALID_PKT_LENGTH =    0x03
AMDTP_STATUS_INSUFFICIENT_BUFFER =   0x04    # not used
AMDTP_STATUS_UNKNOWN_ERROR =         0x05
AMDTP_STATUS_BUSY =                  0x06
AMDTP_STATUS_TX_NOT_READY =          0x07
AMDTP_STATUS_RESEND_REPLY =          0x08
AMDTP_STATUS_RECEIVE_CONTINUE =      0x09
AMDTP_STATUS_TRANSMIT_CONTINUE =     0x0a
AMDTP_STATUS_RECEIVE_DONE =          0x0b
AMDTP_STATUS_MAX =                   0x0c

# eAmdtpState
AMDTP_STATE_INIT =           0x00     # not used
AMDTP_STATE_TX_IDLE =        0x01
AMDTP_STATE_RX_IDLE =        0x02     # not used
AMDTP_STATE_SENDING =        0x03
AMDTP_STATE_GETTING_DATA =   0x04     # not used
AMDTP_STATE_WAITING_ACK =    0x05
AMDTP_STATE_MAX =            0x06     # not used

# eAmdtpPktType
AMDTP_PKT_TYPE_UNKNOWN =     0x00     # not used
AMDTP_PKT_TYPE_DATA =        0x01
AMDTP_PKT_TYPE_ACK =         0x02
AMDTP_PKT_TYPE_CONTROL =     0x03
AMDTP_PKT_TYPE_MAX =         0x04     #  not used

AMDTP_CONTROL_RESEND_REQ =   0x00
AMDTP_CONTROL_SEND_READY =   0x01

AMDTP_MAX_PAYLOAD_SIZE    =  512      #  match with peripheral side
AMDTP_LENGTH_SIZE_IN_PKT  =  2
AMDTP_HEADER_SIZE_IN_PKT  =  2
AMDTP_CRC_SIZE_IN_PKT     =  4
AMDTP_PREFIX_SIZE_IN_PKT  =  AMDTP_LENGTH_SIZE_IN_PKT + AMDTP_HEADER_SIZE_IN_PKT
AMDTP_PACKET_SIZE         =  AMDTP_MAX_PAYLOAD_SIZE + AMDTP_PREFIX_SIZE_IN_PKT + AMDTP_CRC_SIZE_IN_PKT    # Bytes

class AmdtpClient():

    """
    The Client Interface for Bleak Backend implementations to implement.

    The documentation of this interface should thus be safe to use as a reference for your implementation.

    Keyword Args:
        Received_data_callback:  Call back for final data packet received.
        send_central_callback:   Call back for data to send to peripheral
        debug = True         :   will enable debug messages from library
    """

    def __init__(self, **kwargs):

        self.rxlen = 0                      # size of total packet
        self.rxOffset = 0                   # current number received
        self.rxPktsn = 0                    # data serial number on current package
        self.lastRxPktSn = 0                # last data Serial number received 0 .. 15
        self.NextRxPktSn = 0                # next data Serial number or expect
        self.rxPkt = []                     # set RX buffer
        self.ack_enabled = 0                # if acknowledgement is requested (not used yet)
        self.encrypt_enabled = 0            # if encryption is enabled (not used yet)
        self.rxtype = 0                     # data or ack

        self.txPkt = []                     # packet to send to peripheral
        self.txOffset = 0                   # Needed remember the start of the chunk to send
        self.txLen = 0                      # total length of package to send to peripheral
        self.txPktSn = 0                    # Serial number to use when sending
        self.txState = AMDTP_STATE_TX_IDLE  # sending status
        self.packetcount = 0                # packet / chunk counter
        self.SendingNOtComplete = False     # indicate we are sending in chunks

        self.attMtuSize = ATT_DEFAULT_MTU   # needed to break a package is pieces
        self.NewChunkReceived = 0           # packets received counter
        self.cc = crc()                     # calculate CRC32 on a package (to send or receive)

        self._data_callback     =   kwargs.get("Received_data_callback")
        self._central_callback  =   kwargs.get("send_central_callback")
        self.AMD_debug          =   kwargs.get("debug", False)

    def UpdateMTU(self, value):
        """
            set new MTU size, if new has been agreed
        """
        if value > ATT_MAX_MTU:
            self.attMtuSize = ATT_MAX_MTU
        else:
            self.attMtuSize = value

    def ResetRx(self):
        """
            reset the RX buffer and counter
        """
        self.rxPkt.clear()
        self.rxOffset = 0

    def ResetTx(self):
        """
            reset the TX buffer and counter
        """
        self.txPkt.clear()
        self.txOffset = 0

    def AmdtpPacketHandler(self):
        """
            determine whether received information is data, Ack or control package
        """

        if self.rxtype == AMDTP_PKT_TYPE_DATA:
            """
               Data package received
            """
            self.lastRxPktSn = self.rxPktsn

            self.NextRxPktSn = self.lastRxPktSn + 1

            if self.NextRxPktSn == 16:          # only 4 bits
                self.NextRxPktSn = 0

            self.AmdtpSendReply(AMDTP_STATUS_SUCCESS)

            # call back user program from here
            self._data_callback (self.rxPkt, self.rxOffset)

            # reset the received buffer
            self.ResetRx()

        elif self.rxtype == AMDTP_PKT_TYPE_ACK:
            """
                Acknowledgement on previous sent data to peripheral / server
            """
            status = self.rxPkt[0]

            # reset packet received
            self.ResetRx()

            if (self.txState != AMDTP_STATE_WAITING_ACK):

                if self.AMD_debug:
                    print("Warning : Received an unexpected ACK")

            self.txState = AMDTP_STATE_TX_IDLE

            if (status == AMDTP_STATUS_CRC_ERROR or status == AMDTP_STATUS_RESEND_REPLY):

                # resend packet
                self.AmdtpSendPacketHandler()

            else:
                # increase packet serial number if send successfully
                if status == AMDTP_STATUS_SUCCESS:

                    self.txPktSn += 1

                    if (self.txPktSn == 16):     # max 4 bits part of the header (so count 0 - 15)
                        self.txPktSn = 0

        elif self.rxtype == AMDTP_PKT_TYPE_CONTROL:
            """
                This is used when enable_ACK is enabled

                In case fo multiple packets the AMDTP_CONTROL_SEND_READY
                is an indication that the next package can be send.

                The receiver will echo back the serial number of the
                package
            """
            control = self.rxPkt[0]
            resendPktSn = self.rxPkt[1]

            # reset packet received
            self.ResetRx()

            # we have send an AMDTP_CONTROL_REQ_READY as we have more
            # chunks of data to send so we requested from the peripheral
            # a confirmation that we can send next part
            if control == AMDTP_CONTROL_SEND_READY:

                self.NewChunkReceived = resendPktSn
                self.AmdtpSendPacketHandler()

            elif control == AMDTP_CONTROL_RESEND_REQ:

                if resendPktSn > self.lastRxPktSn:
                    self.AmdtpSendReply(AMDTP_STATUS_RESEND_REPLY)

                elif resendPktSn == self.lastRxPktSn:
                    self.AmdtpSendReply(AMDTP_STATUS_SUCCESS)

                else :

                    if self.AMD_debug:
                        print("Warning : Can not act on Control package resendPktSn = {0}, lastRxPktSn = {1}\n".format(resendPktSn,self.lastRxPktSn))
            else:

                if self.AMD_debug:
                    print("Warning : Unexpected control request  = {0}".format(control))

    def AmdtpReceivePkt(self, data, len):
        """
            Parse received data, combine different AMDTP packages to a single
            data packet and call AmdtpPacketHandler
        """
        dataIdx = 0

        #check for minimum length
        if self.rxOffset == 0 and len < AMDTP_PREFIX_SIZE_IN_PKT:

            if self.AMD_debug:
                print("Incomplete package len {0}".format(len))
            self.AmdtpSendReply(AMDTP_STATUS_INVALID_PKT_LENGTH)
            return AMDTP_STATUS_INVALID_PKT_LENGTH

        if self.rxOffset == 0:

            # get length total data package
            clen = [data[0]]
            clen.append(data[1])
            self.rxlen = int.from_bytes(clen, "little")

            self.NewChunkReceived = 0           #reset multiple chunk package counter

            if self.AMD_debug:
                print("rxlen {0}".format(self.rxlen))

            #parse header
            extractor = int("01000000", 2)
            self.ack_enabled = (data[2] & extractor) >> 6
            #print (bin(data[2]))

            extractor = int("10000000", 2)
            self.encrypt_enabled = (data[2] & extractor) >> 7

            extractor = int("11110000", 2)
            self.rxtype = (data[3] & extractor) >> 4

            extractor = int("00001111", 2)
            self.rxPktsn = (data[3] & extractor)

            if self.AMD_debug:
                print("ack_enabled {0} encrypt_enabled {1}".format(self.ack_enabled, self.encrypt_enabled))
                print("rxtype {0} rxPktsn {1}".format(self.rxtype, self.rxPktsn))

            dataIdx = AMDTP_PREFIX_SIZE_IN_PKT # offset 4

            # check that serial number is correct on data package
            if self.rxtype == AMDTP_PKT_TYPE_DATA:

                if self.NextRxPktSn != self.rxPktsn:

                    if self.AMD_debug:
                        print("Warning : Data packet out of sync : expected {0} got {1}".format(self.NextRxPktSn, self.rxPktsn))

                    """
                        There is no recovery here. The sender (peripheral / server) has already overwritten the TX buffer with the new data. Hence it is only displayed as a warning.
                    """

        # copy new data into buffer and also save crc into it if it's the last packet in a message
        # 4 bytes crc is included in pkt length

        i = dataIdx
        while i < len:
            self.rxPkt.append(data[i])
            i = i + 1

        self.rxOffset = self.rxOffset + i - dataIdx

        # complete package has been received
        if (self.rxOffset >= self.rxlen):

            # get 32 bits peer CRC
            peercrc  = self.rxPkt[self.rxOffset - 4]
            peercrc += self.rxPkt[self.rxOffset - 3] << 8
            peercrc += self.rxPkt[self.rxOffset - 2] << 16
            peercrc += self.rxPkt[self.rxOffset - 1] << 24

            # remove CRC from list
            i = 0

            while i < 4:
                self.rxOffset -= 1
                self.rxPkt.pop(self.rxOffset)
                i += 1

            # calculate CRC on received data
            calcCrc = self.cc.crc32(self.rxPkt, self.rxOffset)

            if peercrc != calcCrc:

                if self.AMD_debug:
                    print("invalid CRC got "+ hex(peercrc) + " calculated " + hex(calcCrc))

                # reset list
                self.ResetRx()

                self.AmdtpSendReply(AMDTP_STATUS_CRC_ERROR)

                return AMDTP_STATUS_CRC_ERROR

            # we have it all, now handle request
            self.AmdtpPacketHandler()

            return AMDTP_STATUS_RECEIVE_DONE

        # not all has been received
        # in case request to send confirmation that package has been received
        if self.rxtype == AMDTP_PKT_TYPE_DATA and self.ack_enabled:

            # count this new packet (the packet count starts with 1)
            self.NewChunkReceived += 1
            self.AmdtpSendControl(AMDTP_CONTROL_SEND_READY, self.NewChunkReceived, 1)

        return AMDTP_STATUS_RECEIVE_CONTINUE

    def AmdtpBuildPkt(self, ptype, encrypted, enableACK, buf, len):
        """
            Create packet to be send to the peripheral
            add header (type, encrypt, ack requered, serial number), add data, add CRC
        """
        header1 = 0
        header2 = 0
        self.ResetTx()

        if ptype == AMDTP_PKT_TYPE_DATA:
            header2 = self.txPktSn
        #
        # Prepare header frame to be sent first
        #
        # length
        self.txLen = len + AMDTP_PREFIX_SIZE_IN_PKT + AMDTP_CRC_SIZE_IN_PKT    # len of data + header & len (4) + CRC (4 ytes)
        self.txPkt = [(len + AMDTP_CRC_SIZE_IN_PKT) & 0xff]                    # LSB
        self.txPkt.append(((len + AMDTP_CRC_SIZE_IN_PKT) >> 8) & 0xff)         # MSB length

        # header
        header2 = header2 | (ptype << 4)                                        # add 4 bit type
        if (encrypted):
            header1 = (1 << 7)                                                  # set encryption bit (not used yet)

        if self.txLen > self.attMtuSize - 3:                                    # length of data + header (4) + CRC (4)
            header1 = header1 | (1 << 6)                                        # set control acknowledgement bit

        self.txPkt.append(header1)                                              # set header
        self.txPkt.append(header2)

        # copy data
        i = 0
        while (i < len):
            self.txPkt.append(buf[i])
            i += 1

        # calculate CRC
        calcCrc = self.cc.crc32(buf, len)

        # add CRC
        self.txPkt.append(calcCrc & 0xff)
        self.txPkt.append((calcCrc >> 8) & 0xff)
        self.txPkt.append((calcCrc >> 16) & 0xff)
        self.txPkt.append((calcCrc >> 24) & 0xff)

    def AmdtpcSendAck(self, ptype, encrypted, enableACK, buf, len ):
        """
            send acknowledgement or control
        """
        # create packet with CRC etc
        self.AmdtpBuildPkt(ptype, encrypted, enableACK, buf, len)

        # send ack or control packet
        self._central_callback(self.txPkt, self.txLen)

    def AmdtpSendReply(self, status):
        """
            create a Ack reply package
        """
        buf = [status]                    # first byte is status
        self.AmdtpcSendAck(AMDTP_PKT_TYPE_ACK, False, False, buf, 1)

    def AmdtpSendControl(self, status, extra_info, len_info):
        """
            create a control package
        """
        buf = [status]                    # first byte is status/ command
        if len_info == 1:                 # but there might be more
            buf.append(extra_info)

        self.AmdtpcSendAck(AMDTP_PKT_TYPE_CONTROL, False, False, buf, len_info + 1)

    def AmdtpSendData(self, buf, len):
        """
            called from user program level with data to be send
            returns :
            -1 if eror
            0 if sucessfull
            1 if sending in chunks of data
        """
        st = self.AmdtpcSendPacket(AMDTP_PKT_TYPE_DATA, False, False, buf, len)

        if self.SendingNOtComplete == True:
            return 1

        if st != AMDTP_STATUS_SUCCESS:
            return -1

        return 0

    def AmdtpSendComplete(self):
        """
            Check that sending (in chunks) has been completed
        """
        if self.SendingNOtComplete == True:
            return False

        return True

    def AmdtpcSendPacket(self, ptype, encrypted, enableACK, buf, len):
        """
            check we are ready to send a data package, make sure it fits the peripheral RX buffer
            build the AMDTP package to be send and pass on to send in chunks
        """
        if self.txState != AMDTP_STATE_TX_IDLE:

            if self.AMD_debug:
                print("Data sending failed, tx state = {0}".format(self.txState))
            return AMDTP_STATUS_BUSY

        if len > AMDTP_MAX_PAYLOAD_SIZE:

            if self.AMD_debug:
                print("Data sending failed, exceed maximum payload, len = {0}.".format(len))
            return AMDTP_STATUS_INVALID_PKT_LENGTH

        # build packet to sent
        self.AmdtpBuildPkt(ptype, encrypted, enableACK, buf, len)

        # send packet
        return self.AmdtpSendPacketHandler()

    def AmdtpSendPacketHandler(self):
        """
            Send a data packet to the central
            breakdown in chunks based on the MTU size
        """
        transferSize = 0
        remainingBytes = 0

        if self.txState == AMDTP_STATE_TX_IDLE:
            self.txOffset = 0
            self.txState = AMDTP_STATE_SENDING
            self.packetcount = 0

        if self.txState == AMDTP_STATE_SENDING:

            if self.SendingNOtComplete == True:
                if self.packetcount != self.NewChunkReceived:
                    print("Warning : packages out of sequence. Expected {0}. got {1}".format(self.packetcount, self.NewChunkReceived))

            # Send small pieces of the packet. It just restricts sending to mtusize -3
            # assumed mtusize is set during _init_() or with UpdateMTU
            if self.txOffset < self.txLen:
                remainingBytes = self.txLen - self.txOffset

                if self.attMtuSize - 3 > remainingBytes:
                    transfersize = remainingBytes
                else:
                    transfersize = self.attMtuSize - 3

                i = 0
                transpkt = []
                while i < transfersize:
                    transpkt.append(self.txPkt[self.txOffset + i])
                    i += 1

                # send data packet to the central
                self._central_callback(transpkt, transfersize)
                self.txOffset += transfersize

                # check whether we are done
                if self.txOffset >= self.txLen:

                    # done sent packet
                    self.txState = AMDTP_STATE_WAITING_ACK
                    self.SendingNOtComplete = False

                    #time.sleep(0.05)  # wait between sending complete packets for peripheral to react
                    return AMDTP_STATUS_SUCCESS

                else:
                    self.packetcount += 1
                    self.SendingNOtComplete = True
                    return AMDTP_STATUS_TRANSMIT_CONTINUE
        else:

            if self.AMD_debug:
                print("Could not send txState is not IDLE")

            return AMDTP_STATUS_TX_NOT_READY
