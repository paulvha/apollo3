# ExactLE HCI for V1 Apollo3 Library
## ===========================================================

## Background
Originally as part of the early V1 versions of the Apollo3 / Artemis Sparkfun Artemis boards
the ExactLE driver for Bluetooth was included. In later versions it was removed.

Given that ArduinoBLE came in later, supporting Mbed and thus the V2 library this is a
cut-down version of ExactLE to connect the HCI layer with the ArduinoBLE. Now ArduinoBLE
can be used on V1.x.x board library.

For that an special version of ArduinoBLE has been made. ArduinoBLE_P has a number of
patches and stability changes applied as well as support Exactly of V1.

## Versioning

### Version January 2023 / paulvha
* even further cut down version of ExactLE to remove as much as unnecessary code as possible.
