This is an overview of the calls in ble_Scan_Connect.h

************** PUBLIC *****************

Ble_scan_connect(BLE& ble, events::EventQueue& event_queue)
This will create the constructor for the class.
ble is the constructor to the MBED BLE instance
event_queue is the pointer to the defined buffer to events

~Ble_scan_connect()
This will shutdown the MBED BLE

run()
This will start the intialisation of the MBED BLE stack and
is the entry point for starting

void SetAdrFilter(uint8_t *filter)
set an address filter if match it will connect automatically
The address needs to be MSB first
Only one address can be provided.

void SetUuidFilter(uint8_t *filter, uint8_t len)
set a filter for service if match it will connect automatically
The service UUID needs to be MSB first.
len is the number of bytes in the UUID ( 2,4,16)
Only one service can be provided.

************** internal /private *****************

on_init_complete()
this is called back after intialistation after some checks it
will can scan

scan()
Set up the scan parameters and start scanning for 'scan_duration' time.
scan_duraction is defined by default for 10 seconds

Clear_scan_list()
THis will clear/initialise the list to contain the received devices addresses
The list will be used to only react on a scanned devices when the report was
received the first time.

onAdvertisingReport()
When a remote devices has been detected this routine is called. It will perform
checks that this is the first time around this report, Advertising or scan request,
was received from this device. If scanned and handled earlier it will only display
a dot.
If new it will display the event and payload information and if connectable and the
report was Advertising it will call fro TryToConnect()

onScanTimeout()
This will be called when the scanning has timed out. This will print some statistics by calling
print_scanning_performance() and then call StartAgain().

StartAgain()
Will ask whether the user wants to start scanning again or stop. scan() is called to restart.

AddToFilterList()
THis will add a new Service UUID in the list of maximum 5

CheckForFilterMatch()
THis will check whether an address filter and/or Service UUID filter was set.
If se it will check for a match. If a match was found it will call for DoConnect()

TryToConnect()
is called from onAdvertisingReport() when an address filter and/or Service UUID filter was
set it will call CheckForFilterMatch(). Else it will ask whether the user wants to connect
and will call DoConnect() in the case. Else it will return.

DoConnect()
called from either TryToConnect() or CheckForFilterMatch() it will stop scanning and start the connection.

end_connection_mode()
if connection takes longer tha 10 seconds this routine is called

onConnectionComplete()
This is called when the connection has been completed. All we do right now is demonstrate disconnect

onDisconnectionComplete()
This is called when the disconnect has been completed. Now we call to ask for scanning again.

Display_advertise_details()
Will dispplay the advertise event details. will return true if device can be connected.

Display_payload()
Will display the payload details

display_value_hex()
Will display the data connect in support of Display_payload().

read_scan_duration_in_ms()
Helper to determin the time a connection took

print_scanning_performance()
Print the scanning details call by onScanTimeOut().

conv_addr()
Will convert and bluetooth address to uint8 so it can used to de-dup.





