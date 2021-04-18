# Software Serial on V2.0.x

## ===========================================================
April 2021 / paulvha

## Background

Many people raised question: where is softwareSerial on V2 of the Apollo3 library?
Sounds like a nice challenge to try. I started off with the version that was written by Nathan Seidle for V1.2.x Apollo3 library. This V1 library is NOT using MBED-OS, but has all the functions in itself.

## Result

After long testing and debugging Software Serial is possible with sending/receiving speed up to 9600 and a sending only speed up to 57600.

Higher speeds are not possible with V2.0.x library as something (MBED-OS?) is impacting the overall performance of the excution process with factor of about 3 to 5. So execution of a user level program on V2 is much slower as it seems that the processor is being busy with something else.

That said, normally you would not notice the difference due to the large processor-idle time a normal program has, however when dealing with microseconds precision it becomes an issue as expained below.

However for many solutions a 9600 baud sending/receiving Software Serial or only sending to 57600 is a good enough solution.

## Installation

1. Copy the complete SofwareSerial-directory in the directory :   2.0.6/libraries

2. In order for this SoftwareSerial to work you will need to make a small adjustment to correct a bug in V2

    The file :  2.0.6/cores/arduino/mbed-bridge/core-implement/CommonInterrupt.cpp
    routine : InterruptInParam::_irq_handler().

    Original code :

<pre>
    case IRQ_RISE:
            if (handler->_rise) {
                handler->_rise(handler->_rise_param);
            }
            break;

    Add a line to original code:

    case IRQ_RISE:
    case (IRQ_FALL | IRQ_RISE):      //<<<<<< add this line to get both
            if (handler->_rise) {
                handler->_rise(handler->_rise_param);
            }
            break;

</pre>

    As this file is NOT included in the MBED-OS pre-compiled library, you can make the change and just recompile the sketch.

    NOTE: On later version of V2.0.6 library this might not be needed anymore. This issue is mentioned as a bug in the V2 library on Github at the time of creating this document and hopefully solved in a future release.

## Versioning

### version April 2021
 * Initial version SoftwareSerial for Library 2.0.6

## More detailed information

### Receiving Software Serial : Different approach
==================================================

In many software serial implementations it detects first the change on RX, start bit, which raises an interrupt. From then on it reads the RX-line every X microseconds until it has received/counted all the expected bits + parity etc. The X-microseconds is depending on the baudrate: 1000000 / baudrate. E.g. with baudrate 9600 will give a bit timing of 1000000/9600  = 104us bit rate and 19200 >> 52us, 38400 >> 26us, 56800 >> 17us, 115200 >> 8us

This Software Serial implementation however is triggered on every change (low to high or high to low). It will remember the time of the first change, which is subtracted from the time when a new interrupt / change happens on the line. By dividing the time difference with the bit rate, you know the number of bits have passed since the last time. It will also remember the level of bit (high or low) after the previous change. So if you are running 9600 baud, the bit time is 104us. If the time delta was 208us, you know 2 bits have passed.

This makes timing better as interrupts get a higher priority on the CPU than a running program timers and there is a time delay between level change on the line and interrupt routine starting.

### What happens on RX-interrupt?
===============================

To give you an idea (once the interrupts are set and enabled) when a level change happens on the GPIO, an interrupt happens with the following steps :

1. in gpio_irq_api.c the routine am_gpio_isr() is called.
2. read the necessary status information from the Apollo3-chip
3. loop to detect which GPIO pin raised the interrupt.
4. call the interrupt handler that was set before.(leaving am_gpio_isr())
5. in MBOS-OS the _isrhandler() was set to be used in InterruptIn file. If you use V1 it will call  am_gpio_isr() in ap3_gpio.cpp
6. Here it will check which USER interrupt handler should be called, based on whether the interrupt was raised in FALLING or RISING.
7. Now the interrupt handler is called in user program ( e.g. Software Serial or sketch).

### The issue on V2.0.6
=======================

It takes around 50 – 55us between level change on the RX line and the user interrupt handler in Software Serial is called.
<pre>
The time between level change and step 1 is close ~5us

* level change   start step 1    read step 2    loop in step 3     start call step 4      start step 7
*    0             +5us            +5us              +31us               +4us                +10us
                     5             10                41                 45                   55us
</pre>

On 9600 baudrate (with a bit time of 104us) there is enough time to handle the interrupt. but on 19200.. the next bit is already passing by when the interrupt handler is just called. The timing is getting completely out of sync.

### Same program / measurement, but now on V1.2.1
=================================================
<pre>
The time between level change and step 1 is close ~1.5us
* level change       start step 1    read step 2    loop in step 3         start step 7
*    0                +1.5us            +3.4us        +5.6us                   +1.2us
                     1.5                4.9           10.5                     11.7us
</pre>

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

WOW….. that is a massive difference… it takes ~55us on V2 while ~11.7us on V1.  Both run on the same ATP board and constant reproducable
For this measurement I have used set/clear digital pin (no Serial prints). The overhead is about 1.5us on V1 and 2us on V3, but corrected
in the timing above

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

### What is causing this ?
==========================

Honestly don’t know (yet..)

I thought maybe the clock speed is lower. I have checked the register settings for the clock. In both situations (V1 and V2) they are the same to run 48MHz. I have used CLKOUT to GPIO7 and in both situations it shows 48Mhz on an external scope.

It does not look that V1 is using BURST mode. I see it can be set in the library, but that code is not called from anywhere. I have set it in a sketch and it was enabled without issues, but the timing stayed the same as mentioned above (both V2 and V1).

Is there debug code included that is causing interrupts and delays? I have disabled all the debugging code (removed the -g option from the compiler).  NO difference in timing. Only the static library of Mbed became 7.5Mb instead of 60Mb!! No difference in upload size to the ATP board however.

Is the compiled code so much more inefficient that in order to do an C++ level instruction, it needs more assembler code to perform the same action ? I looked at the assembler code that is generated. Not much of a difference. I then adjusted to V1 interrupt handler to match the V2 interrupt handler as much as possible. Still running V1 the interrupt handler it then takes ~6us longer to loop (step 3 mentioned above), but it is not explaining the difference.

Is the FPU (set hard for V1 and soft for V2) of influence? NO... compiled V1 with ‘-mfloat-abi=softfp -mfpu=fpv4-sp-d16’ instead of ‘-mfloat-abi=hard’ …  no difference on timing on V1.

Is the RTOS that is running on MBED-OS ?
Is it some housekeeping in MBED-OS ?
Is it Systick()?
is it RTC?
Is it what ?????

I have no idea idea yet.. but for now this Software Serial receive stable on 9600 baud on V2.0.X. Maybe in the near future someone with better tools/debuger than I have can find out and maybe provide a solution. Who knows.. maybe I am lucky..

### Impact on sending data
==========================

When sending a byte the interrupts are NOT used. The bit-timing is handled by a trigger from an Apollo3 timer and then sending the next bit, until all are done.

BUT the issue that code takes longer to execute has an impact. This library will extra adjust the timer setting depending on the baudrate to reduce that impact. However the impact is so high that a timer call + excution of the code takes at least 17us, which means we can handle speed up to 57600. No more.. for now...
