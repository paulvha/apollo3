/*
// This file is subject to the terms and conditions defined in
// file 'LICENSE.md', which is part of this source code package.
*/

#ifndef _ARDUINO_MBED_BRIDGE_CORE_EXTEND_COMMON_H_
#define _ARDUINO_MBED_BRIDGE_CORE_EXTEND_COMMON_H_

#include "mbed.h"

#define PinMode Arduino_PinMode         // note: this changes the Arduino API for mbed compatibility - use Arduino_PinMode where PinMode was specified in the Arduino API
#include "core-api/api/Common.h"
#undef PinMode

void            indexPinMode(pin_size_t index, Arduino_PinMode pinMode);
void            pinMode(PinName pinName, Arduino_PinMode pinMode);

void            indexDigitalWrite(pin_size_t index, PinStatus val);
void            digitalWrite(PinName pinName, PinStatus val);

PinStatus       indexDigitalRead(pin_size_t index);
PinStatus       digitalRead(PinName pinName);

int indexAnalogRead(pin_size_t index);
int analogRead(PinName pinName);

//************************************************************************
// BEGIN : ADDED FOR FAST READING ADC PINS
//************************************************************************
# define MAXPINS 5                          // keep track of how many pins

struct ADC_Turbo  {
  PinName pinName;
  uint16_t adc;
};
uint16_t analogReadFast(PinName pinName, bool ReInit = false);
uint16_t analogReadTurbo(ADC_Turbo *res, uint8_t NumPin, bool ReInit = false);
uint16_t analogReadSingleFast(PinName pinName, bool ReInit = false);
//************************************************************************

void            indexAnalogWriteDAC(pin_size_t index, int val);
void            analogWriteDAC(PinName pinName, int val);
void            analogWriteDAC(pin_size_t pinNumber, int val);

void            indexAnalogWrite(pin_size_t index, int val);
void            analogWrite(PinName pinName, int val);

void indexShiftOut(pin_size_t data_index, pin_size_t clock_index, BitOrder bitOrder, uint8_t val);
void shiftOut(PinName dataPinName, PinName clockPinName, BitOrder bitOrder, uint8_t val);

pin_size_t indexShiftIn(pin_size_t data_index, pin_size_t clock_index, BitOrder bitOrder);
pin_size_t shiftIn(PinName dataPinName, PinName clockPinName, BitOrder bitOrder);

#define interrupts() (void)am_hal_interrupt_master_disable()
#define noInterrupts() (void) am_hal_interrupt_master_enable()

void indexAttachInterrupt(pin_size_t index, voidFuncPtr callback, PinStatus mode);
void attachInterrupt(PinName pinName, voidFuncPtr callback, PinStatus mode);

void indexAttachInterruptParam(pin_size_t index, voidFuncPtrParam callback, PinStatus mode, void* param);
void attachInterruptParam(PinName pinName, voidFuncPtrParam callback, PinStatus mode, void* param);

void indexDetachInterrupt(pin_size_t index);
void detachInterrupt(PinName pinName);

#ifdef __cplusplus

void            indexTone(pin_size_t index, unsigned int frequency, unsigned long duration = 0);
unsigned long   indexPulseIn(pin_size_t index, uint8_t state, unsigned long timeout = 1000000L);
unsigned long   pulseIn(PinName pinName, uint8_t state, unsigned long timeout = 1000000L);
unsigned long   indexPulseInLong(pin_size_t index, uint8_t state, unsigned long timeout = 1000000L);
unsigned long   pulseInLong(PinName pinName, uint8_t state, unsigned long timeout = 1000000L);

#if DEVICE_INTERRUPTIN

namespace arduino {

class InterruptInParam : public mbed::InterruptIn {
private:
protected:
public:
    /** Create an InterruptIn connected to the specified pin
     *
     *  @param pin InterruptIn pin to connect to
     */
    InterruptInParam(PinName pin);

    /** Create an InterruptIn connected to the specified pin,
     *  and the pin configured to the specified mode.
     *
     *  @param pin InterruptIn pin to connect to
     *  @param mode Desired Pin mode configuration.
     *  (Valid values could be PullNone, PullDown, PullUp and PullDefault.
     *  See PinNames.h for your target for definitions)
     *
     */
    InterruptInParam(PinName pin, PinMode mode);

    virtual ~InterruptInParam();

    /** Attach a function to call when a rising edge occurs on the input
     *
     *  @param func A pointer to a void function with argument, or 0 to set as none
     */
    void rise(mbed::Callback<void(void*)> func, void* param);

    /** Attach a function to call when a rising edge occurs on the input
     *
     *  @param func A pointer to a void function, or 0 to set as none
     */
    void rise(mbed::Callback<void()> func);   // shadows InterruptIn::rise

    /** Attach a function to call when a falling edge occurs on the input
     *
     *  @param func A pointer to a void function with argument, or 0 to set as none
     */
    void fall(mbed::Callback<void(void*)> func, void* param);

    /** Attach a function to call when a falling edge occurs on the input
     *
     *  @param func A pointer to a void function, or 0 to set as none
     */
    void fall(mbed::Callback<void()> func);   // shadows InterruptIn::fall

    static void _irq_handler(uint32_t id, gpio_irq_event event);

protected:
    mbed::Callback<void(void*)> _rise;    // shadows InterruptIn::_rise
    mbed::Callback<void(void*)> _fall;    // shadows InterruptIn::_fall

    void* _rise_param = nullptr;
    void* _fall_param = nullptr;

    void irq_init(PinName pin);     // shadows InterruptIn::irq_init
};

} // namespace arduio

#endif //DEVICE_INTERRUPTIN

#endif // __cplusplus

#endif // _ARDUINO_MBED_BRIDGE_CORE_EXTEND_COMMON_H_
