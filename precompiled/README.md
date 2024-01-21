# Using precompiled libraries

## ===========================================================

## Background

There was a post that precompiled libraries are not working with Artemis/ Apollo3. I wondered why
As such I did a deepdive to understand, analyze precompiled libraries and find a solution for the Sparkfun Artemins/Apollo3 V2.2.1 library.
<br>
The issue is NOT because of the Arduino-IDE... it is a combination of Sparkfun's library and the compiler.
<br>
Included is a document that can be opened by any word processor providing insight in
<br>
   * pros and cons for pre-compiled libraries

   * developing and using pre-compiled libraries with Arduino-IDE

   * what changes to apply for Sparkfun's Apollo3 boards library to make it to work

<br>
Next to that find the adjusted boards.txt and platform.txt for the V2.2.1 Sparkfun's Apollo3 boards library if you do not want to make the changes yourself.
<br>
This has been tested and working on both 1.18.16 and V 2.2.2 Arduino-IDE
<br>
## Versioning

### version 1.0.1 / January 2024
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

