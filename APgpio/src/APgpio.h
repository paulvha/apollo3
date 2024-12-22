
/* header file file for apollo3 GPIO control
 *
 * @brief Functions for accessing and configuring the GPIO module.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  For detailed explanation see included README.md
 *
 *  Version 1.0 / May 2021 / Paulvha
 */
#include "Arduino.h"

// define the different modes
enum Mode {
	M_VDD = 0,
	M_VSS,
	M_INPUT,
	M_OUTPUT,
	M_PULLUP,
	M_PULLDOWN
};

// define the different pull-up modes (K ohm)
enum Resistor {
	R_1_5 = 1,
	R_6 = 6,
	R_12 = 12,
	R_24 = 24
};

// define the different strength (mA)
enum Dstrength {
	D_2 = 2,
	D_4 = 4,
	D_8 = 8,
	D_12 = 12
};

// user addressable functions
bool APmode(uint32_t pad, Mode mode, int arg = 0);
bool APset(uint32_t pad, int val);
bool APread(uint32_t pad);
