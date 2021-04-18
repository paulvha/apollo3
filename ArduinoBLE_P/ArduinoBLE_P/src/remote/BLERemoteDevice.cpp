/*
  This file is part of the ArduinoBLE library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

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

#include "BLERemoteDevice.h"

BLERemoteDevice::BLERemoteDevice()
{
}

BLERemoteDevice::~BLERemoteDevice()
{
  clearServices();
}

void BLERemoteDevice::addService(BLERemoteService* service)
{
  service->retain();        // increase reference count

  _services.add(service);   // add to list BLELinkedList..
}

unsigned int BLERemoteDevice::serviceCount() const
{
  return _services.size();
}

BLERemoteService* BLERemoteDevice::service(unsigned int index) const
{
  return _services.get(index);
}

void BLERemoteDevice::clearServices()
{
  for (unsigned int i = 0; i < serviceCount(); i++) {
    BLERemoteService* s = service(i);

    if (s->release() <= 0) {        // decrease reference count and maybe delete RemoteAttribute.
      delete s;
    }
  }

  _services.clear();        // remove from services BLELinkedList
}