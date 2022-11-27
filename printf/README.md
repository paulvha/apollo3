
# Printf floating point on V2.2.1
## ===========================================================

## Background

The Apollo3 V2.x.x library is build around MBED 5. For speed reasons the MBED code is taken from a
pre-compiled archive. However the trade of is that some default values are used and one of them is
the use of MBED_MINIMAL_PRINTF. As a result trying to do display a floating point with printf
will fail. This is a solution to that problem.

It provides 3 new routines
<br> Artemis_printf
<br> Artemis_sprintf
<br> Artemis_dtostrf

These are described in the printf.h file.

## Software installation

See install.txt for detailed installation description.
There is also a example sketch to test.

## versioning
### Version 1.0 / November 2022 / paulvha
 * intial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## Disclaimer
Please be fully aware that when using any open source code, like this one, **you are on your own: No Support, No warranty, NO help with damage**. In legal terms :

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Acknowledgments
Thanks to Eyal Rozenberg and Marco Paland for the printf.c routines

