 /*
 * This example will scan for other Bluetooth Devices for a certain
 * amount of time and will display for each unique detected device
 * the scan information.
 *
 * It will then ask whether you want to connect. You can also set
 * a device address and/or service UUID and /or device name.
 * It will not ask and if either matches it will connect.
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
 * based on :
 *
 * https://github.com/ARMmbed/mbed-os-ble-utils
 *
 * https://github.com/ARMmbed/mbed-os-example-ble/tree/master/BLE_GAP
 *
 * The extended developement by paulvha /Version 1.0 / January 2022
 */
#include "mbed_ble.h"

#ifndef BLE_SCANNER_CONNECT_H
#define BLE_SCANNER_CONNECT_H

/* Type response reports */
enum ScanType {
  TYPE_NONE = 0,
  TYPE_ADVERTISE,
  TYPE_RESPONSE
};

/* structure to de-dup incoming advertising or scan request reports */
struct ScanReport {
  uint8_t     addr[6];        // holds the address of the discovered peer device
  ScanType    scanresponse;   // if set it will the scan report
};

// Maximum number of unique peer device reports
// a single device can provide an scan request response after an advertise response
// These reports will be tracked seperately
#define MAXSCANS 20

// store the unique scans
struct ScanReport scans[MAXSCANS];

// maximum length of service UUID (16 bytes = 128 bits)
#define FILTERSIZE 16

// maximum filter UUID's to store from advertising / scan request
#define MAXUUID 5

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
    true,                            /* enable active scanning to also receive the (optional) scan response (next to advertising) package*/
    ble::own_address_type_t::PUBLIC  // use PUBLIC as it will fail otherwise with RANDOM
);

// set for 10 seconds scan
static const ble::scan_duration_t scan_duration(ble::millisecond_t(10000));

/* Delay start scan 1s*/
static const std::chrono::milliseconds DelayStep = 1000ms;

// set for 10 seconds connect timeout
static const std::chrono::milliseconds Connect_duration = 10000ms;

/** scanning & connecting*/
class Ble_scan_connect : private mbed::NonCopyable<Ble_scan_connect>, public ble::Gap::EventHandler
{

public:

  Ble_scan_connect(BLE& ble, events::EventQueue& _event_queue) :
      _ble(ble),
      _gap(ble.gap()),
      _event_queue(_event_queue)
  {
  }

  ~Ble_scan_connect()
  {
    if (_ble.hasInitialized()) {
        _ble.shutdown();
    }
  }

  /** Start BLE interface initialisation */
  void run()
  {
    /* handle gap events so all onxxxx() event routines
     * defined in this class will be called when necessary */
    _gap.setEventHandler(this);

    ble_error_t error = _ble.init(this, &Ble_scan_connect::on_init_complete);

    if (error) {
      print_error(error, "Error returned by BLE::init");
      while(1);
    }

    /* this will not return until shutdown */
    _event_queue.dispatch_forever();
  }

  //////////////////////////////////////////////////////////////////////
  // filter routines
  //////////////////////////////////////////////////////////////////////
  /** set an address filter, if match it will connect automatically
   * MSB first !!
   */
  void SetAdrFilter(uint8_t *filter)
  {
    for (uint8_t i = 0; i < 6 ; i++) {
      _Filter_address[i] = filter[i];
    }

    _FilterSet = true;
  }

  /** set a filter for service, if match it will connect automatically
   * MSB first !!
   */
  void SetUuidFilter(uint8_t *filter, uint8_t len)
  {
    uint8_t i, j;
    _ulen = len;

    if (_ulen > FILTERSIZE) _ulen = FILTERSIZE;

    for (i = 0 ; i < _ulen ; i++) {
      _Filter_uuid[i] = filter[i];
    }

    _FilterSet = true;
  }

  /* set a filter for a device name,  if match it will connect automatically */
  void SetNameFilter(String filter)
  {
    uint8_t  _nlen = filter.length();

    if ( _nlen > FILTERSIZE) _nlen = FILTERSIZE;

    for (uint8_t i = 0 ; i < _nlen ; i++) {
      _Filter_name[i] = filter[i];
    }

    _FilterSet = true;
  }

private:

  /** This is called when BLE interface is initialised and can start scanning*/
  void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
  {
    if (event->error) {
      print_error(event->error, "Error during the initialisation");
      while(1);
    }

    print_mac_address();

    /* all calls are serialised on the user thread through the event queue */
    _event_queue.call_in(DelayStep,[this]{ scan(); });
  }

  /** Set up and start scanning */
  void scan()
  {
    printf("Starting scanning\r\n");

    ble_error_t error = _gap.setScanParameters(scan_params);

    if (error) {
      print_error(error, "Error caused by Gap::setScanParameters");
      return;
    }

    // initialise parameters
    _scan_count = 0;          // total discovered (including multiple times)
    _scan_unique_count = 0;   // unique devices discovered
    _count_dots = 0;          // display dots on screen for multiple discovered
    Clear_scan_list();        // initialise the unique scanned list

    // start scanning for a certain amount of time
    error = _gap.startScan(scan_duration);
    if (error) {
      print_error(error, "Error caused by Gap::startScan");
      return;
    }

    printf("\r\nScanning started (interval: %dms, window: %dms, timeout: %dms).\r\n",
      scan_params.get1mPhyConfiguration().getInterval().valueInMs(),
      scan_params.get1mPhyConfiguration().getWindow().valueInMs(),
      scan_duration.valueInMs());

    _demo_duration.reset();
    _demo_duration.start();
  }

  /* init structure / clear unique scanned list */
  void Clear_scan_list()
  {
    for (uint8_t i=0; i < MAXSCANS; i++) {
        BdaClr(scans[i].addr);
        scans[i].scanresponse = TYPE_NONE;
    }
  }

  /** Called when a peer device has been scanned */
  void onAdvertisingReport(const ble::AdvertisingReportEvent &event) override
  {
    uint8_t i, temp_addr[6], n;
    bool found = false;             // is address unique
    bool TryConnect = false;        // if the device is connectable
    bool PayLoadHeader = true;      // enable payload header

    // waiting for user input, no action nor display to be taken on Advertising report
    if (_AskingUserInput) return;

    /* keep track of scan events for performance reporting */
    _scan_count++;

    /* only look at events from devices at a close range */
    if (event.getRssi() < -65) return;

    // get the peeraddress
    conv_addr(&temp_addr[0],event.getPeerAddress());

    // obtain report type: Advertising (false) or scan request reponse (true)
    const ble::advertising_event_t cc = event.getType();
    ScanType ScanReportType  = cc.scan_response() ? TYPE_RESPONSE : TYPE_ADVERTISE;

    // check the discovered device is not seen before,
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

    _count_dots = 0;              // count restart next time at beginning

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
    else {    // add this new address & report type to the list
      BdaCpy(scans[i].addr,temp_addr);
      scans[i].scanresponse = ScanReportType;
    }

    // display advertising event details
    // Returns true if connectable
    TryConnect = Display_advertise_details(event);

    ble::AdvertisingDataParser adv_parser(event.getPayload());

    // in case of name match will be reset if miss-match
    _FilterNameMatch = true;

    /* parse the advertising payload */
    while (adv_parser.hasNext()) {

      ble::AdvertisingDataParser::element_t field = adv_parser.next();

      // display
      Display_payload(field, PayLoadHeader);

      PayLoadHeader = false;

      // if filter was set
      if(_FilterSet) {

        // if current field is name
        if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME || field.type == ble::adv_data_type_t::SHORTENED_LOCAL_NAME) {

          // if name filter was set check for match
          if (_nlen > 0) {

            uint8_t n = field.value.size();

            if ((n == _nlen) && (memcmp(field.value.data(), _Filter_name, n) == 0)) {
              _FilterNameMatch = true;
            }
            else
              _FilterNameMatch = false;
          }
        }

        // if current field is a UUID
        if (field.type  >= ble::adv_data_type_t::INCOMPLETE_LIST_16BIT_SERVICE_IDS  && field.type <= ble::adv_data_type_t::COMPLETE_LIST_128BIT_SERVICE_IDS)
            AddToFilterList(field);
      }
    }

    printf("////////////// END /////////////\r\n\n");

    // try to connect only on the advertise report
    if (ScanReportType == TYPE_ADVERTISE) {

      if(TryConnect) TryToConnect(event);
      else printf("Device is not connectable\r\n");
    }
  }

  /* when scanning is timing out */
  void onScanTimeout(const ble::ScanTimeoutEvent&) override
  {
    // if we try to connect, we stop scan first
    if (_is_connecting) {
      printf("\r\nStopped scanning\r\n");
      return;
    }

    printf("\r\nStopped scanning due to timeout parameter\r\n");

    print_scanning_performance();

    ble_error_t error = _gap.stopScan();

    if (error) {
        print_error(error, "Error caused by Gap::stopScan");
    }
    _event_queue.call_in(DelayStep,this,&Ble_scan_connect::StartAgain);

  }

  void StartAgain()
  {
    // stop handling any other report coming in
    _AskingUserInput = true;

    // ask with non-blocking input
    String Question = "Do you want to restart scanning (y/n)";
    cb = &Ble_scan_connect::scan;    // what routine to call on Yes
    _cancel_handle = _event_queue.call_every(DelayStep,[this,Question] {AskInput(Question);} );
  }

//////////////////////////////////////////////////////////////////////
// filter routines
//////////////////////////////////////////////////////////////////////

  /* convert address_t to uint8_t */
  inline void conv_addr( uint8_t *dst, const ble::address_t &addr )
  {
    for (uint8_t i = 0; i < 7; i++)
      dst[i] = addr[i];
  }

  /* check for filter match and if so, connect */
  void CheckForFilterMatch(const ble::AdvertisingReportEvent &event)
  {
    bool MatchFound = false;
    uint8_t tempA[6], i, j;

    // check whether name filter was sent if so copy _FilterNameMatch
    if(_nlen > 0)
    {
      MatchFound = _FilterNameMatch;
    }

    // check whether the address filter was set
    // if match was found we are already going to connect, no need to check
    if ((_Filter_address[0] != 0x0 || _Filter_address[1] != 0x0) && !MatchFound )
    {
      // get the peeraddress
      conv_addr(&tempA[0],event.getPeerAddress());

      for (i = 0; i < 6 ; i++) {
        if (tempA[i] != _Filter_address[i])  break;
      }

      if (i == 6) MatchFound = true;  //  match
    }

    // check whether the filter UUID was set and no match found yet
    // if match was found we are already going to connect, no need to check
    if (_ulen > 0 && !MatchFound)
    {
      // if any UUID received from peer
      if (_NumUuid > 0x0)
      {
        // each received UUID
        for (j = 0; j < _NumUuid ; j++){

          // check on UUID match
          for (i = 0; i < _UUIDs[j].ulen;  i++) {
             if (_UUIDs[j].u[i] != _Filter_uuid[i]) { // no match
              _NumUuid = 0;
              return;
            }
          }

          // was length the same a for the filter (needle)
          if ( i == _ulen ) MatchFound = true;  //  match
          break;
        }
      }
    }

    // reset count received UUID from Peer
    _NumUuid = 0;

    // so we found a match
    if (MatchFound) DoConnect();
  }

  /* add new UUID to search list
   * MAXUUID unique UUID's can be added to the list from a peer device
   */
  void AddToFilterList(ble::AdvertisingDataParser::element_t field)
  {
    uint8_t n = field.value.size();

    // save UUID received
    for (int c=n-1; c > -1 ;c--) _UUIDs[_NumUuid].u[c] = (uint8_t) field.value[c];

    // save length of UUID received ( 2, 4 or 16 bytes )
    _UUIDs[_NumUuid].ulen = n;

    // set for next entry else stay/overwrite last entry next time
    if ( _NumUuid < MAXUUID-1) _NumUuid++;
  }

//////////////////////////////////////////////////////////////////////////
// connect routines
/////////////////////////////////////////////////////////////////////////

  /** Try to connect. If a filter was set no question will be asked */
  void TryToConnect(const ble::AdvertisingReportEvent &event)
  {
    if (_is_connecting) return;

    // save the information about the device we potentially want to connect to as
    // event area will be overwritten when new reports arrive

    _peerAddressType = event.getPeerAddressType();
    const ble::address_t &p = event.getPeerAddress();
    ble::address_t &d = _peerAddress;
    for (uint8_t i =0; i < 6 ;i++) d[i] = p[i];

    // if filter was set check for match and if so it will connect
    // no question asked
    if (_FilterSet) {
      CheckForFilterMatch(event);
      return;
    }

    // ask with non-blocking input
    _AskingUserInput = true;
    String Question = "Do you want to try to connect (y/n)";
    cb = &Ble_scan_connect::DoConnect;    // what routine to call on Yes

    // AskInput is called every DelayStep time
    _cancel_handle = _event_queue.call_every(DelayStep,[this,Question] {AskInput(Question);} );
  }

  /* This routine will ask for input non-blocking
   *  param Question : is the question to ask to get a Y or N
   *  param CB : is the function that will be called if the answer is yes
   *
   *  On No there is no further action taken
   */
  void AskInput(String Question)
  {
    static bool StartHeader = true;

    if (! _AskingUserInput) return;

    if (StartHeader){
        // clear any pending input
        while (Serial.available()) Serial.read();
        Serial.print(Question);
        StartHeader = false;
    }

    if (!Serial.available()) return;
    char c = Serial.read();

    Serial.println(c);

    if (c == 'y' || c == 'Y' ) {
      _event_queue.cancel(_cancel_handle);     // stop calling AskInput
      _AskingUserInput = false;               // unblock incoming reports
      StartHeader = true;                     // Display header next time around
      _event_queue.call_in(DelayStep,this,cb); // call the wanted function
    }
    else if (c == 'N' || c == 'n' ){
      Serial.print("OK, as you wish.. no action\r\n");
      _event_queue.cancel(_cancel_handle);     // stop calling AskInput
    }
    else // expected yes or no. reprint question
      StartHeader = true;
  }

  /** perform a connection to the device last scanned */
  void DoConnect()
  {
     ble_error_t error = _gap.stopScan();

     if (error) {
      print_error(error, "Error caused by Gap::stopScan");
     }

    /* we may have already scan events waiting to be processed so we need to remember
     * that we are already connecting and ignore them */
      _is_connecting = true;

    // trying to connect
    _cancel_handle = _gap.connect(
      _peerAddressType,          // source: event.getPeerAddressType(),
      _peerAddress,              // source: event.getPeerAddress(),
      ble::ConnectionParameters()
        /*.setScanParameters(
            ble::phy_t::LE_1M,
            ble::scan_interval_t(96), // max
            ble::scan_window_t(48)    //min
        )
        .setConnectionParameters(
            ble::phy_t::LE_1M,
            ble::conn_interval_t(6),     // minConnectionInterval
            ble::conn_interval_t(12),    // maxConnectionInterval
            ble::slave_latency_t(0),
            ble::supervision_timeout_t(200),
            ble::conn_event_length_t(4),  // minEventLength
            ble::conn_event_length_t(6)   // maxEventLength
        )*/
        .setOwnAddressType(ble::own_address_type_t::PUBLIC)  // use PUBLIC as it will fail otherwise with RANDOM
    );

    /* this will stop connection if it takes longer than Connect_duration*/
    _cancel_handle_connect = _event_queue.call_in(Connect_duration, this, &Ble_scan_connect::end_connection_mode);

    printf("connection process started\r\n");
    _demo_duration.reset();
    _demo_duration.start();
  }

  /* when timeout is happening on connection */
  void end_connection_mode()
  {
    printf("ERROR : Timeout happened when trying to connect\r\n");

    _is_connecting = false;

    /* cancel the connect request*/
    _event_queue.cancel(_cancel_handle);

    // Ask to restart scanning
    _event_queue.call_in(DelayStep,this,&Ble_scan_connect::StartAgain);
  }

  /** This is called by Gap to notify the application we connected */
  void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
  {
    _is_connecting = false;
    _demo_duration.stop();

    if (event.getStatus() != BLE_ERROR_NONE) {
      print_error(event.getStatus(), "Connection failed");
      return;
    }

    printf("Connected in %dms\r\n", read_in_ms());

    /* cancel the connect timeout since we connected */
    _event_queue.cancel(_cancel_handle_connect);

    _cancel_handle = _event_queue.call_in(
        DelayStep,
        [this, handle=event.getConnectionHandle()]{
            _gap.disconnect(handle, ble::local_disconnection_reason_t::USER_TERMINATION);
        }
    );
  }

  /** This is called by Gap to notify the application we disconnected */
  void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
  {
      printf("Disconnected\r\n");

      // if it wasn't us disconnecting then we should cancel our attempt
      if (event.getReason() == ble::disconnection_reason_t::REMOTE_USER_TERMINATED_CONNECTION) {
        printf("Disconnect triggered by remote device\r\n");
        _event_queue.cancel(_cancel_handle);
      }
      else
        printf("Disconnected by this program\r\n");

      // Ask to restart scanning
      _event_queue.call(this, &Ble_scan_connect::StartAgain);
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

private:
  BLE &_ble;
  ble::Gap &_gap;
  events::EventQueue &_event_queue;
  int _cancel_handle, _cancel_handle_connect;
  void (Ble_scan_connect::*cb)();

  /* Measure performance of our scanning */
  mbed::Timer _demo_duration;
  size_t _scan_count = 0;
  size_t _scan_unique_count = 0;
  size_t _count_dots = 0;

  // impact flow
  volatile bool _is_connecting = false;
  volatile bool _AskingUserInput = false;

  // save the information of the device you want to connect to
  ble::address_t _peerAddress;
  ble::peer_address_type_t _peerAddressType;

  // filter variables
  struct UUIDr {
    uint8_t u[FILTERSIZE];            // uuid received from peer
    int     ulen;                     // length of UUID received
  };

  bool _FilterSet = false;            // a filter request was set (needle)

  uint8_t _Filter_address[6];         // holds the bdaddr to filter on (needle)
  uint8_t _Filter_uuid[FILTERSIZE];   // holds the UUID to filter on (needle)
  uint8_t _ulen = 0;                  // length of UUID (needle)
  char    _Filter_name[FILTERSIZE];   // holds the peer name filter
  uint8_t _nlen = 0;                  // length of name (needle)
  bool    _FilterNameMatch = false;   // did peer name and filtername match
  uint8_t _NumUuid = 0;               // number of UUID registered from peer (haystack)
  struct UUIDr _UUIDs[MAXUUID];       // holds UUID received from peer(haystack)
};

#endif //BLE_SCANNER_CONNECT_H
