/* mbed Microcontroller Library
 * Copyright (c) 2018 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file has additional changes needed for Artemis / Apollo
 *
 * version 1.0 / December 2021 / paulvha
 */

#ifndef PRETTY_PRINTER_H_
#define PRETTY_PRINTER_H_

#include <mbed.h>
#include "ble/BLE.h"
#include "ble/gap/AdvertisingDataParser.h"

// prototypes
inline void print_address(const ble::address_t &addr);
inline const char* phy_to_string(ble::phy_t phy);

/* structure to select manufacturer name */
struct man {
  uint16_t id;
  char     name[10];
};

// maximum number of entries for manufacturer
#define MAX_MAN_ENTRIES 20

// info from https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/
struct man ManToName[MAX_MAN_ENTRIES] = {
  {0x004c, "Apple Inc"},
  {0x0006, "Microsoft"},
  {0x0065, "HP inc"},
  {0x01DA, "Logitec"},
  {0x00E0, "Google"},
  {0x0075, "Samsung"},
  {0x02DE, "Samsung"},
  {0x0029, "Hitachi"},
  {0x0087, "Garmin"},
  {0xFFFF, "end"}         // Add more as you like, update MAX_MAN_ENTRIES if needed but this entry must be at end of list
};

/**
 * will display the advertise details
 * @param event is provided by onAdvertisingReport()
 *
 * return
 *  true if device is connectable
 *  false it is not connectable
 */
bool Display_advertise_details(const ble::AdvertisingReportEvent &event)
{
  bool connectable = false;
  printf("===================================\r\n");
  printf("       TYPE INFORMATION\r\n");
  printf("===================================\r\n");
  // Defined in Version 5.0 | Vol 2, Part E -
  // Section 7.7.65.13 LE Extended Advertising report Event
  ble::advertising_event_t a =  event.getType();    // events.h

  if (a.connectable()){
    printf("Device is connectable, scannable and doesn't expect connection from a specific peer\r\n");
    connectable = true;
  }
  if (a.scannable_advertising())
    printf("Device is scannable and doesn't expect connection from a specific peer\r\n");

  if (a.directed_advertising()){
    printf("Directed advertising types accept connection requests from a known peer device\r\n");
    printf("\r\nThe direct address: ");
    print_address(event.getDirectAddress());
  }

  if (a.scan_response())     printf("scan response\r\n");
  if (a.legacy_advertising())printf("legacy_advertising\r\n");
  if (a.complete())          printf("Response is complete\r\n");
  if (a.more_data_to_come()) printf("more_data_to_come\r\n");
  if (a.truncated())         printf("truncated\r\n");

  printf("===================================\r\n");
  printf("       EVENT INFORMATION\r\n");
  printf("===================================\r\n");

  printf("TXpower\t\t: %d\r\n",event.getTxPower());
  printf("RSSI\t\t: %d\r\n",event.getRssi());
  printf("SID\t\t: %d\r\n",event.getSID());

  if (event.isPeriodicIntervalPresent()){
    printf("Periodic advertising\r\n");
    printf("\tFixed interval: %d\r\n",event.getPeriodicInterval() );
  }

  if (event.getPeerAddressType() == ble::peer_address_type_t::PUBLIC)
    printf("PeerAddressType\t: 0x0 Public Device Address\r\n");
  else
    printf("PeerAddressType\t: 0x01 Random Device Address\r\n");

  printf("Primary Phy\t: %s\r\n",phy_to_string(event.getPrimaryPhy()));

  // secondary might be set, so we display NONE
  printf("Secondary Phy\t: %s\r\n",phy_to_string(event.getSecondaryPhy()));

  return(connectable);
}

inline void print_error(ble_error_t error, const char* msg)
{
    printf("%s: ", msg);
    switch(error) {
        case BLE_ERROR_NONE:
            printf("BLE_ERROR_NONE: No error");
            break;
        case BLE_ERROR_BUFFER_OVERFLOW:
            printf("BLE_ERROR_BUFFER_OVERFLOW: The requested action would cause a buffer overflow and has been aborted");
            break;
        case BLE_ERROR_NOT_IMPLEMENTED:
            printf("BLE_ERROR_NOT_IMPLEMENTED: Requested a feature that isn't yet implement or isn't supported by the target HW");
            break;
        case BLE_ERROR_PARAM_OUT_OF_RANGE:
            printf("BLE_ERROR_PARAM_OUT_OF_RANGE: One of the supplied parameters is outside the valid range");
            break;
        case BLE_ERROR_INVALID_PARAM:
            printf("BLE_ERROR_INVALID_PARAM: One of the supplied parameters is invalid");
            break;
        case BLE_STACK_BUSY:
            printf("BLE_STACK_BUSY: The stack is busy");
            break;
        case BLE_ERROR_INVALID_STATE:
            printf("BLE_ERROR_INVALID_STATE: Invalid state");
            break;
        case BLE_ERROR_NO_MEM:
            printf("BLE_ERROR_NO_MEM: Out of Memory");
            break;
        case BLE_ERROR_OPERATION_NOT_PERMITTED:
            printf("BLE_ERROR_OPERATION_NOT_PERMITTED");
            break;
        case BLE_ERROR_INITIALIZATION_INCOMPLETE:
            printf("BLE_ERROR_INITIALIZATION_INCOMPLETE");
            break;
        case BLE_ERROR_ALREADY_INITIALIZED:
            printf("BLE_ERROR_ALREADY_INITIALIZED");
            break;
        case BLE_ERROR_UNSPECIFIED:
            printf("BLE_ERROR_UNSPECIFIED: Unknown error");
            break;
        case BLE_ERROR_INTERNAL_STACK_FAILURE:
            printf("BLE_ERROR_INTERNAL_STACK_FAILURE: internal stack failure");
            break;
        case BLE_ERROR_NOT_FOUND:
            printf("BLE_ERROR_NOT_FOUND");
            break;
        default:
            printf("Unknown error");
    }
    printf("\r\n");
}

/*  printf on Artemis does not seem to like %02X as it prints one position for 0x0 to 0xf */
void print_num_hex(uint8_t n)
{
   if (n < 0x10) printf("0%X",n);
   else printf("%X",n);
}

/**
 * Print the value of a UUID.
 */
void print_uuid(const UUID &uuid)
{
   const uint8_t *uuid_value = uuid.getBaseUUID();

   // UUIDs are in little endian, print them in big endian
   for (size_t i = 0; i < uuid.getLen(); ++i) {
     //printf("%02X", uuid_value[(uuid.getLen() - 1) - i]);
     print_num_hex(uuid_value[(uuid.getLen() - 1) - i]);
   }
}


/**
 * Print the value of a characteristic properties.
 */

typedef DiscoveredCharacteristic::Properties_t Properties_t;
void print_properties(const Properties_t &properties)
{
  const struct {
      bool (Properties_t::*fn)() const;
      const char* str;
  } prop_to_str[] = {
      { &Properties_t::broadcast, "broadcast" },
      { &Properties_t::read, "read" },
      { &Properties_t::writeWoResp, "writeWoResp" },
      { &Properties_t::write, "write" },
      { &Properties_t::notify, "notify" },
      { &Properties_t::indicate, "indicate" },
      { &Properties_t::authSignedWrite, "authSignedWrite" }
  };

  printf("[");
  for (size_t i = 0; i < (sizeof(prop_to_str) / sizeof(prop_to_str[0])); ++i) {
      if ((properties.*(prop_to_str[i].fn))()) {
          printf(" %s", prop_to_str[i].str);
      }
  }
  printf(" ]");
}

/** print device address to the terminal */
inline void print_address(const ble::address_t &addr)
{
  for (int z = 5 ; z > -1 ; z--){

    print_num_hex(addr[z]);

    if(z != 0) printf(":");
  }

  printf("\r\n");
}


/* display value in Hex
 * field: element
 * rev  : little Endian so display reverse
 * st   : offset in value
 */
void display_value_hex(ble::AdvertisingDataParser::element_t field, bool rev = false, uint8_t st = 0)
{
  uint8_t n = field.value.size();

  // if reverse / little endian
  if (rev) {
    for (int c=n-1; c > st -1 ;c--) print_num_hex((uint8_t) field.value[c]);
  }
  else {
    for (int c=st; c<  n ;c++) print_num_hex((uint8_t) field.value[c]);
  }

   printf("\r\n");
}

/* print manufacturer name from ID */
void print_manufact(uint16_t m)
{

  for(uint8_t r = 0; r < MAX_MAN_ENTRIES; r++){

    // last entry
    if (ManToName[r].id == 0xFFFF) return;

    // match
    if(ManToName[r].id == m) {
      printf(" (%s)", ManToName[r].name);
      return;
    }
  }
}


/* display payload
 * I order to use in sketch do something like :
 *
  bool PayLoadHeader = true;
  ble::AdvertisingDataParser adv_parser(event.getPayload());

  while (adv_parser.hasNext()) {

   ble::AdvertisingDataParser::element_t field = adv_parser.next();

   // display
   Display_payload(field, PayLoadHeader);

   PayLoadHeader = false;
 }
*/
void Display_payload(ble::AdvertisingDataParser::element_t field, bool PayLoadHeader)
{

  if(PayLoadHeader){
    printf("===================================\r\n");
    printf("       PAYLOAD INFORMATION\r\n");
    printf("===================================\r\n");
  }

  if (field.type == ble::adv_data_type_t::FLAGS) {
    printf("FLAGS\r\n");

    if (ble::adv_data_flags_t(field.value[0]).getGeneralDiscoverable()) {
      printf("\tGeneralDiscoverable\r\n");
    }
    if (ble::adv_data_flags_t(field.value[0]).getlimitedDiscoverable()) {
      printf("\tlimitedDiscoverableoverable\r\n");
    }
    if (ble::adv_data_flags_t(field.value[0]).getBrEdrNotSupported()) {
      printf("\tBrEdrNotSupported\r\n");
    }
    if (ble::adv_data_flags_t(field.value[0]).getSimultaneousLeBredrH()) {
      printf("\tSimultaneousLeBredrH (Host)\r\n");
    }
    if (ble::adv_data_flags_t(field.value[0]).getSimultaneousLeBredrC()) {
      printf("\tSimultaneousLeBredrC (Controller)\r\n");
    }

  }
  else if (field.type == ble::adv_data_type_t::INCOMPLETE_LIST_16BIT_SERVICE_IDS) {
    printf("INCOMPLETE_LIST_16BIT_SERVICE_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::COMPLETE_LIST_16BIT_SERVICE_IDS) {
    printf("COMPLETE_LIST_16BIT_SERVICE_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::INCOMPLETE_LIST_32BIT_SERVICE_IDS) {
    printf("INCOMPLETE_LIST_32BIT_SERVICE_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::COMPLETE_LIST_32BIT_SERVICE_IDS) {
    printf("COMPLETE_LIST_32BIT_SERVICE_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::INCOMPLETE_LIST_128BIT_SERVICE_IDS) {
    printf("INCOMPLETE_LIST_128IT_SERVICE_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::COMPLETE_LIST_128BIT_SERVICE_IDS) {
    printf("COMPLETE_LIST_128BIT_SERVICE_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::SHORTENED_LOCAL_NAME) {
    printf("SHORTENED_LOCAL_NAME\r\n\t");
    uint8_t n = field.value.size();
    for (uint8_t c=0; c < n ;c++)  printf("%c", field.value[c]);
    printf("\r\n");
  }
  else if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME) {
    printf("COMPLETE_LOCAL_NAME\r\n\t");
    uint8_t n = field.value.size();
    for (uint8_t c=0; c < n ;c++) printf("%c", field.value[c]);
    printf("\r\n");
  }
  else if (field.type == ble::adv_data_type_t::TX_POWER_LEVEL) {
    printf("TX_POWER_LEVEL\r\n\t");
    printf("level %d dBm\r\n",ble::adv_data_flags_t(field.value[0]));
  }
  else if (field.type == ble::adv_data_type_t::DEVICE_ID) {
    printf("DEVICE_ID\r\n\t");
    display_value_hex(field);
  }
  else if (field.type == ble::adv_data_type_t::SLAVE_CONNECTION_INTERVAL_RANGE) {
    printf("SLAVE_CONNECTION_INTERVAL_RANGE\r\n");
  }
  else if (field.type == ble::adv_data_type_t::LIST_16BIT_SOLICITATION_IDS) {
    printf("LIST_16BIT_SOLICITATION_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::LIST_128BIT_SOLICITATION_IDS) {
    printf("LIST_128BIT_SOLICITATION_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::SERVICE_DATA) {
    printf("SERVICE_DATA\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::SERVICE_DATA_16BIT_ID) {
    printf("SERVICE_DATA_16BIT_ID\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::SERVICE_DATA_128BIT_ID) {
    printf("SERVICE_DATA_128BIT_ID\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::LIST_16BIT_SOLICITATION_IDS) {
    printf("LIST_16BIT_SOLICITATION_IDS\r\n\t");
    display_value_hex(field,true,0);
  }
  else if (field.type == ble::adv_data_type_t::APPEARANCE) {
    printf("APPEARANCE\r\n\t");
  }
  else if (field.type == ble::adv_data_type_t::ADVERTISING_INTERVAL) {
    printf("ADVERTISING_INTERVAL\r\n\t");
  }
  else if (field.type == ble::adv_data_type_t::MANUFACTURER_SPECIFIC_DATA) {

    printf("MANUFACTURER_SPECIFIC_DATA\r\n\t");

    printf("Number: 0x");

    //manufacturer number
    print_num_hex((uint8_t) field.value[1]);
    print_num_hex((uint8_t) field.value[0]);

    // try to determine manufacturer name
    uint16_t o = field.value[1]<< 8 |  field.value[0];
    print_manufact(o);

    // device info
    printf("\r\n\tDevice: 0x");
    display_value_hex(field,false,2);

  }
  else {
    printf("what ?? %x \r\n", field.type );
  }
  printf("\r\n");

}

inline void print_mac_address()
{
    /* Print out device MAC address to the console*/
    ble::own_address_type_t addr_type;
    ble::address_t address;
    BLE::Instance().gap().getAddress(addr_type, address);
    printf("Device MAC Address: ");
    print_address(address);
}


inline const char* phy_to_string(ble::phy_t phy) {
    switch(phy.value()) {
        case ble::phy_t::NONE:
            return "None";
        case ble::phy_t::LE_1M:
            return "LE 1M";
        case ble::phy_t::LE_2M:
            return "LE 2M";
        case ble::phy_t::LE_CODED:
            return "LE coded";
        default:
            return "invalid PHY";
    }
}

#endif /* PRETTY_PRINTER_H_ */
