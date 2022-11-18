
# Fast ADC reading Apollo3 V 1.2.3
## ===========================================================

## Background

The Apollo3 library V1.2.3 has the standard analogRead() included. While this works well it contains a lot of overhead with each call to make sure that right data is obtained. As a result the maximum frequency to obtain ADC reading is 12Khz. This is enough for many if not most implementations. However there are situations where a faster speed is required. This add-on will enable that.

In analogReadTurbo() & analogReadFast() & analogReadSingleFast() many of the checks are removed and expects the usage is only to read the ADC value on one of the 10 ADC pins. Not all pins are exposed external on all Apollo3 boards. See the board pinout information on Sparkfun.com.

analogReadTurbo() and analogReadFast() are mend to be used to read multiple ADC pins in a fast manner. An Apollo3 has 8 ADC slots that can be used. In analogReadTurbo() and analogReadFast() setup each pin will get its own ADC-Slot assigned and enabled. With each read-cyle all the enabled slots are read in one go. Reading a maximum of 5 ADC-pins in parallel possible (set by default in the adjusted driver)

analogReadSingleFast() is a modified analogRead() optimized to perform a fast reading of a SINGLE ADCpin. It can achieve a very high frequency, but only on a single ADCPin.

## Software installation
Follow the steps to copy 2 files to the right place as defined in the install.txt

## Prerequisites
This only works on V1 apollo3 library from Sparkfun. It has been tested on V1.2.3

## Examples
4 examples are available:
* Example1_V1 : compare the different functions and  speeds.
* Example2_V1 : usage of analogReadTurbo()
* Example3_V1 : usage of analogReadFast()
* Example4_V1 : usage of analogReadSingleFast()

Due to differences in the V1-library the V2 examples do not work.

## Versioning
### Version 1.0 / November 2022
* initial multi-read ADC version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## Disclaimer
Please be fully aware that when using any open source code, like this one, **you are on your own: No Support, No warranty, NO help with damage**. In legal terms :

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Acknowledgments
Thanks to the Apollo3 datasheet

