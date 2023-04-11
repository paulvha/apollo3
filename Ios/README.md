# Apollo3 IOS

## ===========================================================

IOS stands for Input Output Slave. In short to use a board with an Apollo3 chip as a slave device  either supporting SPI or I2C communication. It is the counter part of the Apollo3 Input Output Master (IOM).

In the SparkFun library there is no implemented use for IOS while in the AMBIQ SDK there are 2 examples. Both of them have the major function defined in the Hardware Abstraction Layer (HAL), but not all of them.

I wondered why there is no better implementation and also wanted to better understand how a slave device would work.

Hence this project. I’ll share my learning, observation and make sure to read my conclusion.

paulvha
April 2023

## Getting Started
There are 4 examples:

<br>Example1_IOS_I2C_Peripheral, to be loaded on an Artemis ATP board.
<br>Example1_IOS_I2C_host, can be loaded to many different boards.

<br>Example2_IOS_SPI_Peripheral, to be loaded on an Artemis ATP board.
<br>Example1_IOS_SPI_host, can be loaded to many different boards.

<br>See top of sketches for the hardware connections.

## Prerequisites
Examples depend on the SparkFun BME280 library

## Software installation
Copy this folder to SparkFun/hardware/apollo3/libraries

## Program usage
Please see the description in the top of the sketch and read the documentation Apollo3-IOS.odt. This can be openend with nearly any word document processor.

## Versioning

### Version 1.0.0 / April 2023
 * Initial version for V1.2.3 and V2.2.1

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

## Acknowledgments

### Make sure to read the datasheet from Apollo3.<br>
### parts taken from the Ambiq SDK examples ios_fifo.c ios_fofo_host.c / ios_lram.c <br>



