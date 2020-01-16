# Artemis / Apollo3 example

## ===========================================================

Example of ADC sampling battery supply voltage divider, battery load resistor,
and read the internal temperature in Celsius and/or Fahrenheit.

This example is using the complex way to get these values as it will initialize
and handle the ADC registers itself, instead of relying on the Arduino Apollo3
library to handle and perform analogRead() calls.

This was the result of a deep study to understand how the ADC implementation of
the Apollo3 was working to get a better understanding of the ADC module in the Apollo chip.

In the included Readme.h I try to explain what is happening

## Versioning

### version 1.0 / January 2020
 * Initial version tested with the edge board
 * batt_temp will read local battery level, temperature and set battery load resistor
 * batt_temp is NOT using the standard analogRead(), but intializes the ADC module it self

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements
Make sure to have the Apollo3 datasheet from https://cdn.sparkfun.com/assets/learn_tutorials/9/0/9/Apollo3_Blue_MCU_Data_Sheet_v0_9_1.pdf
