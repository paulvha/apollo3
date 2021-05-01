# GPIO input /output settings
## ===========================================================

## Background

The Apollo3 chip has different GPIO setting to adjust the input and output pad. However due to the “Arduino-like” compatibility, not all of these options can be addressed with the default / standard code. This code is working directly with the HAL (Hardware Abstraction Layer) to perform different settings. The code has been tested on both the V1.2.1 and V2.0.6 boards library from Sparkfun.

## Disclaimer

During testing the code I accidentally damaged one of the pad (41) on my ATP board. Oh well..XXX happens. BUT I want to warn you to be careful and be fully aware that using any open source code, like this one, **you are on your own: No Support, No warranty, NO help with damage**. In legal terms :

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Pin or Pad

A “pin” is the name of the place where you connect your wire on a board. A pad is the connection point of the Apollo3 processor to the board. While “pin” does not have to be the same as a “pad” on Apollo3 board it nearly always is. Often there is confusion whether to use a pinName (like D3) or a pin / pad-number (like 3). When addressing the HAL it is always the pad-number, the layer(s) above will translate a pinName to a pad-number.

## Commands available

There are 3 commands available:

### APmode(pad,mode)
This will set a pad in an certain mode
Pad    : a number between 0 and 50
Mode   :
	M_VDD 		enable power Switch to 3v3 on other pad 3 or 36
	M_VSS,		enable power Switch to 3v3 on other pad 41 or 37
	M_INPUT,	set pad as input
	M_OUTPUT,   set pad as output (*note 1)
	M_PULLUP,   set pad as input with pullup resistor (*note 2)
	M_PULLDOWN  set pad with pulldown (only pad 20 is allowed)

Return : TRUE if succeeded, False if failed

*note1 : An additional argument can be passed to set strength as 2mA, 4mA, 8mA or 12mA*
*note2 : An additional argument can be passed to set pullup resistor 1.5K, 6K, 12K, 24K*

### APset(pad, level)
This will set a level on an output pad (set previously with APmode())
Pad    : a number between 0 and 50
mode   : HIGH or LOW
Return : TRUE if succeeded, False if failed.

### APread(pad)
This will read the input level from a pad.
Pad    : a number between 0 and 50
return : True = HIGH, False = LOW

## Software installation
Copy the APgpio-directory in the Sparkfun boards library folder:
xxx/libraries. xxx is 1.2.1 or 2.0.6 (depending on the version).
In this libraries folder you also find others like SPI or Wire.

## Prerequisites
Example4 is depending on ![BME280](https://github.com/sparkfun/SparkFun_BME280_Arduino_Library)

## Examples
4 examples are available:
Example1 : set three different inputs
Example2 : set output
Example3 : Set power switch output
Example4 : Set Power switch output for BME280

## Versioning
### Version 1.0 / May 2021
* initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## Acknowledgments
Thanks to the APollo3 datasheet
