/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2018 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "BLEUuid.h"
#include "HCI.h"

#include "GAP.h"

// only 5 scan devices at any time the queue
#define GAP_MAX_DISCOVERED_QUEUE_SIZE 5

#define GAP_ADV_IND (0x00)
#define GAP_ADV_SCAN_IND (0x02)
#define GAP_ADV_NONCONN_IND (0x03)
#define SCAN_RSP (0x04)

GAPClass::GAPClass() :
  _advertising(false),
  _scanning(false),
  _advertisingInterval(160),
  _connectable(true),
  _discoverEventHandler(NULL)
{
}

GAPClass::~GAPClass()
{
}

bool GAPClass::advertising()
{
  return _advertising;
}

int GAPClass::advertise(uint8_t* advData, uint8_t advDataLen, uint8_t* scanData, uint8_t scanDataLen)
{
  uint8_t directBdaddr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  uint8_t type = (_connectable) ? GAP_ADV_IND : (scanDataLen ? GAP_ADV_SCAN_IND : GAP_ADV_NONCONN_IND);
  stopAdvertise();

  if (HCI.leSetAdvertisingParameters(_advertisingInterval, _advertisingInterval, type, 0x00, 0x00, directBdaddr, 0x07, 0) != 0) {
    return 0;
  }

  if (HCI.leSetAdvertisingData(advDataLen, advData) != 0) {
    return 0;
  }

  if (HCI.leSetScanResponseData(scanDataLen, scanData) != 0) {
    return 0;
  }

  if (HCI.leSetAdvertiseEnable(0x01) != 0) {
    return 0;
  }

  _advertising = true;

  return 1;
}

void GAPClass::stopAdvertise()
{
  _advertising = false;

  HCI.leSetAdvertiseEnable(0x00);
}
// if withDuplicates is true.. it means that the scan will NOT filter out duplicates
// so you get many advertise reports
int GAPClass::scan(bool withDuplicates)
{
  // clear out any pending discovered device list before starting new scan
  unsigned int dd = _discoveredDevices.size();
  for (unsigned int i = 0; i < dd; i++) {
    BLEDevice* device = _discoveredDevices.remove(i);
    if (device != NULL) {
      delete device;
    }
  }

  HCI.leSetScanEnable(false, true);

  // active scan, 10 ms scan interval (N * 0.625), 10 ms scan window (N * 0.625), public own address type, no filter
  if (HCI.leSetScanParameters(0x01, 0x0010, 0x0010, 0x00, 0x00) != 0) {
    return false;
  }

  _scanning = true;

  if (HCI.leSetScanEnable(true, !withDuplicates) != 0) {
    return 0;
  }

  return 1;
}

int GAPClass::scanForName(String name, bool withDuplicates)
{
  _scanNameFilter    = name;
  _scanUuidFilter    = "";
  _scanAddressFilter = "";

  return scan(withDuplicates);
}

int GAPClass::scanForUuid(String uuid, bool withDuplicates)
{
  _scanNameFilter    = "";
  _scanUuidFilter    = uuid;
  _scanAddressFilter = "";

  return scan(withDuplicates);
}

int GAPClass::scanForAddress(String address, bool withDuplicates)
{
  _scanNameFilter    = "";
  _scanUuidFilter    = "";
  _scanAddressFilter = address;

  return scan(withDuplicates);
}

void GAPClass::stopScan()
{
  HCI.leSetScanEnable(false, false);

  _scanning = false;

  for (unsigned int i = 0; i < _discoveredDevices.size(); i++) {
    BLEDevice* device = _discoveredDevices.get(i);

   if (device != NULL) {
      delete device;
    }
  }

  _discoveredDevices.clear();
}

// checking for new peripheral added.
// e.g. from an advertise report.
BLEDevice GAPClass::available()
{
  for (unsigned int i = 0; i < _discoveredDevices.size(); i++) {
    BLEDevice* device = _discoveredDevices.get(i);

    if (device->discovered()) {
      BLEDevice result = *device;

      // remove from list
      _discoveredDevices.remove(i);

      // this is freeing up memory, but we are still going to use it ??
      delete device;

      // if there was a match on a filter or NO filter set
      if (matchesScanFilter(result)) {
        return result;
      } else {
        continue;
      }
    }
  }

  return BLEDevice();
}

void GAPClass::setAdvertisingInterval(uint16_t advertisingInterval)
{
  _advertisingInterval = advertisingInterval;
}

void GAPClass::setConnectable(bool connectable)
{
  _connectable = connectable;
}

// optional to set a call back when scan data has been received
void GAPClass::setEventHandler(BLEDeviceEvent event, BLEDeviceEventHandler eventHandler)
{
  if (event == BLEDiscovered) {
    _discoverEventHandler = eventHandler;
  }
}

// advertising report (event 0x3e, subevent 0x02)
void GAPClass::handleLeAdvertisingReport(uint8_t type, uint8_t addressType, uint8_t address[6],
                                          uint8_t eirLength, uint8_t eirData[], int8_t rssi)
{
  if (!_scanning) {
    return;
  }


/* if a scan call back was set & advertise report
 * 0x03 = Non-Connectable Non-Scannable Undirected advertising (ADV_NONCONN_IND)
 * you can not connect nor ask for a scanreport. info in that packet only.
 * like a beacon..
 */
  if (_discoverEventHandler && type == GAP_ADV_NONCONN_IND) {

    // create a device.
    BLEDevice device(addressType, address);

    device.setAdvertisementData(type, eirLength, eirData, rssi);

    // if there is a match on a filter or filter not set
    if (matchesScanFilter(device)) {
      _discoverEventHandler(device);
    }

    return;
  }

  BLEDevice* discoveredDevice = NULL;
  int discoveredIndex = -1;

  // check we have already added this peripheral earlier to the list
  for (unsigned int i = 0; i < _discoveredDevices.size(); i++) {
    BLEDevice* device = _discoveredDevices.get(i);

    if (device->hasAddress(addressType, address)) {
      discoveredDevice = device;
      discoveredIndex = i;

      break;
    }
  }

  // if not see before
  if (discoveredDevice == NULL) {
    // can we still add? if not....
    if (_discoveredDevices.size() >= GAP_MAX_DISCOVERED_QUEUE_SIZE) {
    // ********* begin change ******
/*      // drop
      return;
    }
*/
     // https://github.com/arduino-libraries/ArduinoBLE/pull/264 fix BLE scanning block after minutes
     // we need to free up
     BLEDevice* device_first = _discoveredDevices.remove(0);

     if (device_first != NULL) {
       delete device_first;
     }
   }
   // ******** end change *************
   // create new device
   discoveredDevice = new BLEDevice(addressType, address);

   // add to list
   _discoveredDevices.add(discoveredDevice);
   discoveredIndex = _discoveredDevices.size() - 1;
  }

  if (type != SCAN_RSP) {   // if NOT Scan Response (0x04) and thus advertise data
    discoveredDevice->setAdvertisementData(type, eirLength, eirData, rssi);
  } else {
    // copy Scan Response AFTER advertising data
    discoveredDevice->setScanResponseData(eirLength, eirData, rssi);
  }

  // Only scanresponse (0x04) and a scan call back was set
  if (discoveredDevice->discovered() && _discoverEventHandler) {

    BLEDevice device = *discoveredDevice;

    // callback with handler if it matches the filter or filter NOT set
    if (matchesScanFilter(device)) {
      _discoverEventHandler(device);
    }

    // remove from list
    _discoveredDevices.remove(discoveredIndex);

    // free up memory
    delete discoveredDevice;
  }
}

// check for any filter that might have been set
bool GAPClass::matchesScanFilter(const BLEDevice& device)
{
  if (_scanAddressFilter.length() > 0 && !(_scanAddressFilter.equalsIgnoreCase(device.address()))) {
    return false; // drop doesn't match
  } else if (_scanNameFilter.length() > 0 && _scanNameFilter != device.localName()) {
    return false; // drop doesn't match
  } else if (_scanUuidFilter.length() > 0 && !(_scanUuidFilter.equalsIgnoreCase(device.advertisedServiceUuid()))) {
    return false; // drop doesn't match
  }

  return true;
}

#if !defined(FAKE_GAP)
GAPClass GAPObj;
GAPClass& GAP = GAPObj;
#endif
