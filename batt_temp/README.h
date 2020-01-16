/*
Short explanation / description of the driver
*********************************************

version 1.0 / January 2020 / paulvha

This driver will read the power supply / battery level and temperature using 
the Apollo3 internal mechanism.

This is described in chapter 18 ADC and Temperature Sensor Module of
the Apollo3 datasheet. The ADC has 15 user-selectable channels with sources including:
- External pins
-   10 single ended
-   2 differential pairs
- Internal voltage (VSS)
- Voltage divider (battery)
- Temperature sensor

Each channel can be connected to one of 8 slots where the real conversion / compare
is taking place.

The ADC logic needs to be initialized with the call ADC_INIT().
First a handle is created to access the logic (am_hal_adc_initialize), then the
logic is woken up (am_hal_adc_power_control)
Next step is to configure the ADC logic with settings that will apply to
all channels with am_hal_adc_configure().
Next is to assign the slots. By setting 'sSlotCfg.bEnabled = false'  the slot
is disabled and not included in the conversion cycle nor does it not provide
output to the FIFO. This is applied for slots 0 to 5. Slot 6 and 7 are enabled,
where slot 6 is assined to the battery channel (AM_HAL_ADC_SLOT_CHSEL_BATT)
and slot 7 to Temperature (AM_HAL_ADC_SLOT_CHSEL_TEMP).  Both will only do
one conversion (AM_HAL_ADC_SLOT_AVG_1). Slot 6 and 7  are instructed to do a 14 BIT ADC,

One could set upper and lower limits and have the results compared against
them (bWindowCompare). If the result is outside these boundaries an interrupt
could be raised. We have not implemented that.
As a last point the ADC is enabled (am_hal_adc_enable)

By calling GET_BATT_VALUE() we can obtain the battery level. It will call adc_read()
(see below) to get the ADC value from battery. The value is actually coming from a
voltage divider (10K / 5k) and the provided level is 1/3 of the real value.
To calculate the real value we have to multiple the ADC value by 3 AND by the value
for each step. The value for each ADC step is the reference software / 14BIT ADC.

One of the features of the Apollo3 chip is to provide a load resistor on the
the battery (~500 ohm) to stress test the battery. This can be set or removed by
BATTERY_LOAD(). In the sketch it is set or removed each loop as an example.

By calling GET_TEMP_VALUE() we can obtain the Temperature in either celsius or
Fahrheneheit. It will call adc_read() (see below) to get the ADC value from battery.
The value for each ADC step is the reference software / 14BIT ADC.

As part of the Apollo3 logic it can adjust the ADC reading to provide a more accurate
result. This is called TRIM. Either these adjustment values are specific to this
chip and stored in the NV-memory, or these are default values. Use print_state() to
what is the case for you.

By calling am_hal_adc_control() it will calculate the temperature in Celsius based
on the current reading + apply the TRIM.
As a last action either the value in celsius or in Fahrenheit is returned

ADC_READ() is an supporting call for the user routines. It will trigger a conversion
cycle (am_hal_adc_sw_trigger) for as much samples as indicated  and then waits for 
completion by reading (am_hal_adc_interrupt_status)
It will then read from the FIFO an entry as long as there is one
(AM_HAL_ADC_FIFO_COUNT(ADC->FIFO)). We only expect 2 entries (slot 6 and 7)
as the others where disabled.

*/
