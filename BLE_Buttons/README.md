# Sending button code over BLE to Artemis /Apollo3

## ===========================================================
This version is based on the BLE-LED, but it has one service (0000183B-08C2-11E1-9073-0E8AC72E1011) with one characteristic (0000183B-08C2-11E1-9073-0E8AC72E0011).

## Program usage
By writing a value to the characteristic it will call void button_received(uint8_t b) which is in the sketch.
<br> For test I have defined 4 buttons and in button_received() it just matches to the defines to print the button detected.
<br> I have tested with nRF-connect on my mobile phone and wrote a value of 0x11 and 0x12. It showed the upbutton and downbutton.
I have not tried to make a fancy program around it, this is more example on how it could work.

## CO-Author
 * Paul van Haastrecht (paulvha@hotmail.com)

<br>No Support, no warranty, no obligations. Just source code as-is !

