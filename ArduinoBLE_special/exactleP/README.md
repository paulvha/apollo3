# Exactle for V1 Apollo3 Library
## ===========================================================

## Background
Originally as part of the early V1 versions of the Apollo3/ Artemis Sparkfun Artemis boards
the Exactle driver for Bluetooth was included. In later versions it was removed.
Given that ArduinoBLE came in later, supporting Mbed and thus the V2 library this is a
special port of Exactly that can be used on V1 libraries and connect with ArduinoBLE.

For that an special version of ArduinoBLE has been made. ArduinoBLE_P has a number of
patches and stability changes applied as well as support Exactly of V1.


## Versioning

### version January 2023 / paulvha
* even further cut down version of Exactle to remove as much as unnecessary code as possible

### version February 2025 / paulvha
* added a call/function to change the BLE signal strength. Default is 0db, you can also select
* minus 10 dbM and plus 3dbM
