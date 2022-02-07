/*
 * Scanner for BLE on Mbed/Cordio
 *
 * This example will scan for other Bluetooth Devices for a certain
 * amount of time and will display for each unique detected device
 * the scan information.
 *
 *  ********************************************************
 *
 * A number of routines are coming from Mbed with the following
 * license information
 *
 * mbed Microcontroller Library
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
 * https://github.com/ARMmbed/mbed-os-ble-utils
 *
 * https://github.com/ARMmbed/mbed-os-example-ble/tree/master/BLE_GAP
 *
 * Other parts have been developed by paulvha
 * Version 1.0 / January 2022
 */

#include "mbed_ble.h"

#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

/* Type response reports received */
enum ScanType {
  TYPE_NONE = 0,
  TYPE_ADVERTISE,
  TYPE_RESPONSE
};

/* structure to de-dup incoming advertising or scan request reports */
struct ScanReport {
  uint8_t     addr[6];        // holds the address of the discovered peer device
  ScanType    scanresponse;   // if set it will indicate the  scan report
};

// Maximum number of unique peer device reports
#define MAXSCANS 20

// store the unique scans
struct ScanReport scans[MAXSCANS];

/* Scanning happens repeatedly and is defined by:
 *  - The scan interval which is the time (in 0.625us) between each scan cycle.
 *  - The scan window which is the scanning time (in 0.625us) during a cycle.
 * If the scanning process is active, the local device sends scan requests
 * to discovered peer to get additional data.
 */
static const ble::ScanParameters scan_params(
    ble::phy_t::LE_1M,
    ble::scan_interval_t(80),
    ble::scan_window_t(60),
    true, /* active scanning enabled */
    ble::own_address_type_t::PUBLIC
);

// set for 10 seconds scan
static const ble::scan_duration_t scan_duration(ble::millisecond_t(10000));

/* Delay start scan*/
static const std::chrono::milliseconds delay_step = 1000ms;

/** Demonstrate scanning*/
class Ble_scan : private mbed::NonCopyable<Ble_scan>, public ble::Gap::EventHandler
{
public:

  Ble_scan(BLE& ble, events::EventQueue& event_queue) :
      _ble(ble),
      _gap(ble.gap()),
      _event_queue(event_queue)
  {
  }

  ~Ble_scan()
  {
    if (_ble.hasInitialized()) {
        _ble.shutdown();
    }
  }

  /** Start BLE interface initialisation */
  void run()
  {
    /* handle gap events so all on_xxxx() event routines
     * defined in this class will be called when necessary */
    _gap.setEventHandler(this);

    ble_error_t error = _ble.init(this, &Ble_scan::on_init_complete);

    if (error) {
      print_error(error, "Error returned by _ble.init");
      while(1);
    }

    /* this will not return until shutdown */
    _event_queue.dispatch_forever();
  }

private:

  /* This is called when BLE interface is initialised and starts scanning*/
  void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
  {
    if (event->error) {
      print_error(event->error, "Error during the initialisation");
      while(1);
    }

    print_mac_address();

    /* all calls are serialised on the user thread through the event queue */
    _event_queue.call_in(delay_step,[this]{ scan(); });
  }

  /** Set up and start scanning */
  void scan()
  {
    printf("Starting scanning\r\n");

    ble_error_t error = _gap.setScanParameters(scan_params);

    if (error) {
      print_error(error, "Error caused by _gap.setScanParameters, freeze");
      while(1);
    }

    // init scan list
    Clear_scan_list();

    /* start scanning and attach a callback that will handle
     * and scan requests responses */
    error = _gap.startScan(scan_duration);
    if (error) {
      print_error(error, "Error caused by _gap.startScan, freeze");
      while(1);
    }

    printf("\r\nScanning started (interval: %dms, window: %dms, timeout: %dms).\r\n",
      scan_params.get1mPhyConfiguration().getInterval().valueInMs(),
      scan_params.get1mPhyConfiguration().getWindow().valueInMs(),
      scan_duration.valueInMs());

    _demo_duration.reset();
    _demo_duration.start();
  }

  /* init structure / clear existing list */
  void Clear_scan_list()
  {
    for (uint8_t i=0; i < MAXSCANS; i++) {
      BdaClr(scans[i].addr);
      scans[i].scanresponse = TYPE_NONE;
    }
  }

  /** Called when a peer device has been scanned */
  void onAdvertisingReport(const ble::AdvertisingReportEvent &event)
  {
    uint8_t i, temp_addr[6], n;
    bool found = false;         // for unique scans
    bool PayLoadHeader = true;  // display PAYLOAD header

    /* keep track of scan events for performance reporting */
    _scan_count++;

    /* only look at events from devices at a close range */
    if (event.getRssi() < -65) {
        return;
    }

    // get the peeraddress
    conv_addr(&temp_addr[0],event.getPeerAddress());

    // get report type: Advertising (false) or scan request reponse (true)
    const ble::advertising_event_t cc = event.getType();
    ScanType ScanReportType  = cc.scan_response() ? TYPE_RESPONSE : TYPE_ADVERTISE;

    // check the discovered device is not seen before.
    for (i = 0 ; i < MAXSCANS; i++){

      // if empty slot: we are at end
      if ( BdaIsZeros(scans[i].addr) ) break;

      // if match to current device address
      if (BdaCmp(scans[i].addr, temp_addr)) {

        // match for type of report
        if (scans[i].scanresponse == ScanReportType) {
          found = true;
          break;
        }
      }
    }

    // already displayed before, skip it
    if (found) {

      Serial.print(".");          // just show we are still receiving scans
      if (_count_dots++ > 75) {
        Serial.println("\r\n");   // keep small lines
        _count_dots = 0;
      }

      return;
    }

    _count_dots = 0;               // restart counting

    printf("\r\n///////////////////////////////////////////////////");

    if (ScanReportType == TYPE_ADVERTISE) {

      printf("\r\nFound a NEW DEVICE at address: ");
      print_address(event.getPeerAddress());

      // count unique devices
      _scan_unique_count++;
    }
    else {
      printf("\r\nADDITIONAL SCAN RESPONSE from address: ");
      print_address(event.getPeerAddress());
    }

    // store in the database
    if ( i == MAXSCANS ) {
      printf("can not add more to de-dup\r\n");
    }
    else {    // now add this to the location
      BdaCpy(scans[i].addr,temp_addr);
      scans[i].scanresponse = ScanReportType;
    }

    // display advertising event details
    Display_advertise_details(event);

    ble::AdvertisingDataParser adv_parser(event.getPayload());

    /* parse the advertising payload */
    while (adv_parser.hasNext()) {

      ble::AdvertisingDataParser::element_t field = adv_parser.next();

      Display_payload(field, PayLoadHeader);

      PayLoadHeader = false;

    }
    printf("////////////// END /////////////\r\n\n");
  }

  /* when scanning is timing out */
  void onScanTimeout(const ble::ScanTimeoutEvent&)
  {
    printf("\r\nStopped scanning due to timeout parameter\r\n");

    print_scanning_performance();

    ble_error_t error = _gap.stopScan();

    if (error) print_error(error, "Error caused by _gap.stopScan");

    _scan_count = 0;
    _scan_unique_count = 0;
    _count_dots = 0;
    Clear_scan_list();

    /* all calls are serialised on the user thread through the event queue */
    _event_queue.call_in(delay_step,[this]{ StartAgain(); });
  }

  /* ask for restarting scan */
  void StartAgain()
  {
    while(1) {

      // clear any pending input
      while (Serial.available()) Serial.read();

      Serial.print("Do you want to restart scanning ?  (y / n) ");

      while (!Serial.available());

      char c = Serial.read();

      Serial.println(c);

      if (c == 'y' || c == 'Y' ) {
        _event_queue.call_in(delay_step,this, &Ble_scan::scan);
        return;
      }
      else if (c == 'n' || c == 'N' ) {
        Serial.println("OK, we are done. BYE !");
        while(1);
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////
  // display routines
  /////////////////////////////////////////////////////////////////////////

  /* helper function to hide the casts */
  int read_in_ms()
  {
    return duration_cast<duration<int, milli>>(_demo_duration.elapsed_time()).count();
  }

  /** print some information about our radio activity */
  void print_scanning_performance()
  {
    /* measure time from mode start, may have been stopped by timeout */
    uint16_t duration_ms = read_in_ms();

    /* convert ms into timeslots for accurate calculation as internally
     * all durations are in timeslots (0.625ms) */
    uint16_t duration_ts = ble::scan_interval_t(ble::millisecond_t(duration_ms)).value();
    uint16_t interval_ts = scan_params.get1mPhyConfiguration().getInterval().value();
    uint16_t window_ts = scan_params.get1mPhyConfiguration().getWindow().value();
    /* this is how long we scanned for in timeslots */
    uint16_t rx_ts = (duration_ts / interval_ts) * window_ts;
    /* convert to milliseconds */
    uint16_t rx_ms = ble::scan_interval_t(rx_ts).valueInMs();

    printf("We had %d scan reports of which %d are unique devices\r\n", _scan_count, _scan_unique_count);
    printf(
        "We have scanned for %dms with an interval of %d"
        " timeslots and a window of %d timeslots\r\n",
        duration_ms, interval_ts, window_ts
    );

   printf("We have been listening on the radio for at least %dms\r\n", rx_ms);
  }

  /* convert address_t to uint8_t */
  inline void conv_addr( uint8_t *dst, const ble::address_t &addr )
  {
    for (uint8_t i = 0; i < 7; i++)
      dst[i] = addr[i];
  }

private:

  BLE &_ble;
  ble::Gap &_gap;
  events::EventQueue &_event_queue;

  /* Measure performance of our scanning */
  mbed::Timer _demo_duration;
  size_t _scan_count = 0;
  size_t _scan_unique_count = 0;

  size_t _count_dots = 0;
};

#endif //BLE_SCANNER_H
