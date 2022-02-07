/** This example demonstrates all the basic setup required to advertise or scan.
 *
 *  It contains a single class that performs BOTH scans and advertisements.
 *
 *  The demonstrations happens in sequence, after each "mode" ends
 *  the demo jumps to the next mode to continue.
 *
 *  You may connect to the device during advertising and if you advertise
 *  this demo will try to connect during the scanning phase.
 *
 *  Connection will terminate the phase early. At the end of the phase some stats
 *  will be shown about the phase.
 *
 * Version 1.0 / paulvha / January 2022
 */

#include "mbed_ble.h"

/* Delay between steps scan or advertise 3 seconds*/
static const std::chrono::milliseconds delay_step = 3000ms;

//program variables to update
const static char DEVICE_NAME[] = "GAP-demo2";

/* demo config */
/* you can adjust these parameters and see the effect on the performance */

/* Advertising parameters are mainly defined by an advertising type and
 * and an interval between advertisements. Lower interval increases the
 * chances of being seen at the cost of increased power usage.
 *
 *
 * Most bluetooth time units are specific to each operation. For example
 * adv_interval_t is expressed in multiples of 625 microseconds. If precision
 * is not require you may use a conversion from milliseconds.
 *
 */
static const ble::AdvertisingParameters advertising_params(

    /* connectable  */
    ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
    ble::adv_interval_t(ble::millisecond_t(25)), /* MIN this could also be expressed as ble::adv_interval_t(40) */
    ble::adv_interval_t(ble::millisecond_t(50))  /* MAX this could also be expressed as ble::adv_interval_t(80) */
  );

static const std::chrono::milliseconds advertising_duration = 10000ms;

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
    false /* active scanning */
);

// 10 seconds
static const ble::scan_duration_t scan_duration(ble::millisecond_t(10000));
/* config end */

/** Demonstrate advertising, scanning and connecting */
class GapDemo : private mbed::NonCopyable<GapDemo>, public ble::Gap::EventHandler
{
public:
    GapDemo(BLE& ble, events::EventQueue& event_queue) :
        _ble(ble),
        _gap(ble.gap()),
        _event_queue(event_queue)
    {
    }

    ~GapDemo()
    {
        if (_ble.hasInitialized()) {
            _ble.shutdown();
        }
    }

    /** Start BLE interface initialisation */
    void run()
    {
        /* handle gap events so all onxxxx() routines
         * defined in this class will be called when necessary
         */
        _gap.setEventHandler(this);

        ble_error_t error = _ble.init(this, &GapDemo::on_init_complete);
        if (error) {
            print_error(error, "Error returned by BLE::init");
            while(1);
        }

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    }


private:
    /** This is called when BLE interface is initialised and starts the first mode */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            print_error(event->error, "Error during the initialisation");
            while(1);
        }

        print_mac_address();    // in pretty_printer.h

        /* all calls are serialised on the user thread through the event queue */
        _event_queue.call_in(delay_step, [this]{ scan(); });
    }

    /** Set up and start scanning */
    void scan()
    {
        printf("starting scanning\r\n");

        ble_error_t error = _gap.setScanParameters(scan_params);
        if (error) {
            print_error(error, "Error caused by Gap::setScanParameters");
            return;
        }

        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
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

    /** Set up and start advertising */
    void advertise()
    {
        ble_error_t error = _gap.setAdvertisingParameters(ble::LEGACY_ADVERTISING_HANDLE, advertising_params);
        if (error) {
            print_error(error, "Gap::setAdvertisingParameters() failed");
            return;
        }

        /* to create a payload we'll use a helper class that builds a valid payload */
        /* AdvertisingDataSimpleBuilder is a wrapper over AdvertisingDataBuilder that allocated the buffer for us */
        ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE> data_builder;

        /* builder methods can be chained together as they return the builder object */
        data_builder.setFlags().setName(DEVICE_NAME);

        /* add some vendor data just for fun*/
        const uint8_t _vendor_specific_data[4] = { 0x50, 0x41, 0x55, 0x4C };
        data_builder.setManufacturerSpecificData(_vendor_specific_data);

        /* Set payload for the set */
        error = _gap.setAdvertisingPayload(ble::LEGACY_ADVERTISING_HANDLE, data_builder.getAdvertisingData());
        if (error) {
            print_error(error, "Gap::setAdvertisingPayload() failed");
            return;
        }

        /* Start advertising the set */
        error = _gap.startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
        if (error) {
            print_error(error, "Gap::startAdvertising() failed");
            return;
        }

        printf(
            "\r\nAdvertising started (type: 0x%x, interval: [%d : %d]ms)\r\n",
            advertising_params.getType(),
            advertising_params.getMinPrimaryInterval().valueInMs(),
            advertising_params.getMaxPrimaryInterval().valueInMs()
        );

        _demo_duration.reset();
        _demo_duration.start();

        /* this will stop advertising if no connection takes place in the meantime */
        _cancel_handle = _event_queue.call_in(advertising_duration, [this]{ end_advertising_mode(); });
    }

    /* helper function to hide the casts */
    int read_demo_duration_in_ms()
    {
        return duration_cast<duration<int, milli>>(_demo_duration.elapsed_time()).count();
    }

private:
    /* Gap::EventHandlers */

    /** Look at scan payload to find a peer device and connect to it
     * as these onxxxx() calls are virtually defined, we only have to define them and not 'set' as well.*/
    void onAdvertisingReport(const ble::AdvertisingReportEvent &event) override
    {
        /* keep track of scan events for performance reporting */
        _scan_count++;

        /* don't bother with analysing scan result if we're already connecting */
        if (_is_connecting) {
            return;
        }

        /* only look at events from devices at a close range */
        if (event.getRssi() < -65) {
            return;
        }

        ble::AdvertisingDataParser adv_parser(event.getPayload());

        /* parse the advertising payload, looking for a discoverable device */
        while (adv_parser.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_parser.next();

            /* skip non discoverable device */
            if (field.type != ble::adv_data_type_t::FLAGS ||
                field.value.size() != 1 ||
                !ble::adv_data_flags_t(field.value[0]).getGeneralDiscoverable()) {
                continue;
            }

            /* connect to a discoverable device */

            /* abort timeout as the mode will end on disconnection */
            _event_queue.cancel(_cancel_handle);

            printf("We found a connectable device at address: ");
            print_address(event.getPeerAddress());

            // if no connection process in progress

            //_is_connecting = true;// for test to overrule connecting
            if (!_is_connecting) {

              // trying to connect
              ble_error_t error = _gap.connect(
                  event.getPeerAddressType(),
                  event.getPeerAddress(),
                  ble::ConnectionParameters() // use the default connection parameters
              );

              if (error) {
                  print_error(error, "Error caused by Gap::connect");
                  return;
              }

              printf("connection process started\r\n");
              _demo_duration.reset();
              _demo_duration.start();

              /* we may have already scan events waiting
               * to be processed so we need to remember
               * that we are already connecting and ignore them */
              _is_connecting = true;
            }
        }
    }

    /* Advertising ends when the process timeout or if it is stopped by the
     * application or if the local device accepts a connection request.*/
     // timeout does not work in current release !!!!!
    void onAdvertisingEnd(const ble::AdvertisingEndEvent &event) override
    {
      ble::advertising_handle_t adv_handle = event.getAdvHandle();

      if (event.isConnected()) {
        printf("Stopped early due to connection\r\n");
      } else {
        printf("Stopped due to user request\r\n");
      }
    }

    void onScanTimeout(const ble::ScanTimeoutEvent&) override
    {
        printf("Stopped scanning due to timeout parameter\r\n");
        _event_queue.call(this, &GapDemo::end_scanning_mode);
    }

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
    {
        _is_connecting = false;
        _demo_duration.stop();

        if (event.getStatus() != BLE_ERROR_NONE) {
            print_error(event.getStatus(), "Connection failed");
            return;
        }

        printf("Connected in %dms\r\n", read_demo_duration_in_ms());

        /* cancel the connect timeout since we connected */
        _event_queue.cancel(_cancel_handle);

        printf("Now disconnecting\r\n");
        _cancel_handle = _event_queue.call_in(
            delay_step,
            [this, handle=event.getConnectionHandle()]{
                _gap.disconnect(handle, ble::local_disconnection_reason_t::USER_TERMINATION);
            }
        );
    }

    /** This is called by Gap to notify the application we disconnected,
     *  in our case it calls next_demo_mode() to progress the demo */
    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
    {
        printf("Disconnected\r\n");

        /* if it wasn't us disconnecting then we should cancel our attempt */
        if (event.getReason() == ble::disconnection_reason_t::REMOTE_USER_TERMINATED_CONNECTION) {
            _event_queue.cancel(_cancel_handle);
        }

        if (_is_in_scanning_phase) {
            _event_queue.call(this, &GapDemo::end_scanning_mode);
        } else {
            _event_queue.call(this, &GapDemo::end_advertising_mode);
        }
    }

    /**
     * Implementation of Gap::EventHandler::onReadPhy
     */
    void onReadPhy(
        ble_error_t error,
        ble::connection_handle_t connectionHandle,
        ble::phy_t txPhy,
        ble::phy_t rxPhy
    ) override
    {
        if (error) {
            printf(
                "Phy read on connection %d failed with error code %s\r\n",
                connectionHandle, BLE::errorToString(error)
            );
        } else {
            printf(
                "Phy read on connection %d - Tx Phy: %s, Rx Phy: %s\r\n",
                connectionHandle, phy_to_string(txPhy), phy_to_string(rxPhy)
            );
        }
    }

    /**
     * Implementation of Gap::EventHandler::onPhyUpdateComplete
     */
    void onPhyUpdateComplete(
        ble_error_t error,
        ble::connection_handle_t connectionHandle,
        ble::phy_t txPhy,
        ble::phy_t rxPhy
    ) override
    {
        if (error) {
            printf(
                "Phy update on connection: %d failed with error code %s\r\n",
                connectionHandle, BLE::errorToString(error)
            );
        } else {
            printf(
                "Phy update on connection %d - Tx Phy: %s, Rx Phy: %s\r\n",
                connectionHandle, phy_to_string(txPhy), phy_to_string(rxPhy)
            );
        }
    }

    /**
     * Implementation of Gap::EventHandler::onDataLengthChange
     */
    void onDataLengthChange(
        ble::connection_handle_t connectionHandle,
        uint16_t txSize,
        uint16_t rxSize
    ) override
    {
        printf(
            "Data length changed on the connection %d.\r\n"
            "Maximum sizes for over the air packets are:\r\n"
            "%d octets for transmit and %d octets for receive.\r\n",
            connectionHandle, txSize, rxSize
        );
    }

private:

    /** Finish the mode by shutting down advertising or scanning and move to the next mode. */
    // called when event times out
    void end_scanning_mode()
    {
        print_scanning_performance();
        ble_error_t error = _gap.stopScan();

        if (error) {
            print_error(error, "Error caused by Gap::stopScan");
        }

        _is_in_scanning_phase = false;
        _scan_count = 0;

        _event_queue.call_in(delay_step, this, &GapDemo::advertise);
    }

    void end_advertising_mode()
    {
       bool start_scan = true;
       print_advertising_performance();

       printf("Requesting stop advertising.\r\n");

       _gap.stopAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

     if (start_scan) {
      _is_in_scanning_phase = true;
      _event_queue.call_in(delay_step, [this]{ scan(); });
     }
    }

    /** print some information about our radio activity */
    void print_scanning_performance()
    {
        /* measure time from mode start, may have been stopped by timeout */
        uint16_t duration_ms = read_demo_duration_in_ms();

        /* convert ms into timeslots for accurate calculation as internally
         * all durations are in timeslots (0.625ms) */
        uint16_t duration_ts = ble::scan_interval_t(ble::millisecond_t(duration_ms)).value();
        uint16_t interval_ts = scan_params.get1mPhyConfiguration().getInterval().value();
        uint16_t window_ts = scan_params.get1mPhyConfiguration().getWindow().value();
        /* this is how long we scanned for in timeslots */
        uint16_t rx_ts = (duration_ts / interval_ts) * window_ts;
        /* convert to milliseconds */
        uint16_t rx_ms = ble::scan_interval_t(rx_ts).valueInMs();

        printf(
            "We have scanned for %dms with an interval of %d"
            " timeslots and a window of %d timeslots\r\n",
            duration_ms, interval_ts, window_ts
        );

        printf("We have been listening on the radio for at least %dms\r\n", rx_ms);
    }

    /** print some information about our radio activity */
    void print_advertising_performance()
    {
        /* measure time from mode start, may have been stopped by timeout */
        uint16_t duration_ms = read_demo_duration_in_ms();

        /* convert ms into timeslots for accurate calculation as internally
         * all durations are in timeslots (0.625ms) */
        uint16_t duration_ts = ble::adv_interval_t(ble::millisecond_t(duration_ms)).value();
        uint16_t interval_ts = advertising_params.getMaxPrimaryInterval().value();
        /* this is how many times we advertised */
        uint16_t events = (duration_ts / interval_ts);
        uint16_t extended_events = 0;

        printf("We have advertised for %dms\r\n", duration_ms);

        /* non-scannable and non-connectable advertising
         * skips rx events saving on power consumption */
        if (advertising_params.getType() == ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED) {
            printf("We created at least %d tx events\r\n", events);
        } else {
            printf("We created at least %d tx and rx events\r\n", events);
        }
    }

private:
    BLE &_ble;
    ble::Gap &_gap;
    events::EventQueue &_event_queue;

    /* Keep track of our progress through demo modes */
    bool _is_in_scanning_phase = false;
    bool _is_connecting = false;

    /* Remember the call id of the function on
     * so we can cancel it if we need to end the phase early */
    int _cancel_handle = 0;

    /* Measure performance of our advertising/scanning */
    mbed::Timer _demo_duration;
    size_t _scan_count = 0;
};
