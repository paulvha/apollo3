
# using MLX90614 on Apollo3

## The challenge

While the Sparkfun Artemis library is I2C compliant, it is NOT SMBus compliant.
SMBus is upper layer specification build on top of I2C.
For complete specification see http://smbus.org/specs/smbus20.pdf
THe MLX90614 is applying most of the SMBus specifications

The major issue is that when the SCL or SDA is longer than 50us idle,
according to SMBus, the bus is deemed to be free. Any earlier received
information needs to be disgarded / reset.

When a read is performed on I2C, first a write of the I2C address +
register_to_read is send, then a second command is send to receive
the content of that register.

On an Artemis V1 library, running clock speed of 200Khz, the delay
AFTER completing sending the write command and starting the read command
is just 50us. On 100Khz it is 88us, much longer than the 50 us and thus
you read incorrect data. On 400Khz that is 38us.

On Artemis V2 library, mainly due to MBED overhead, the delay at
100Khz is 147Khz, at 200Khz 123us and with 400Khz 88us.
See even at the highest clock speed it is MUCH longer than 50us.!!

Just to put things into perspective, on an Arduino Uno the delay is
around 32us, nearly irrespective of the clock speed.

When using V1 library it will only work if the clock speed is 200Khz or higher.
HOWEVER the MLX90614 is only supporting 100Khz, so we are out of luck :-(

## The solution

Next to SMBUS mode (and thus sending on I2C) the MLX90614 has the capability
to send the temperature as a PWM signal. You have to set a number of bits
in configuration registers in the EEPROM, so after power-off / on it
continues to stay in PWM. Once set as PWM the MLX90614 CAN be used on
an Artemis / Apollo3 board either with V1 or V2 library.

## Setting PWM

!!! Setting as PWM OR SMBUS CAN NOT BE DONE ON ARTEMIS / APOLLO3 !!!!
I used my Arduino UNO, but you can also use other AVR-boards.

The sketch 'mlx_change_mode' allows you to switch between SMBus and PWM mode
and back again. As often as you like, although according to the
datasheet you can erase and write the EEPROM only about 100.000 times.

The sketch will have 4 options:
1. Want to change from SMB to PWM?
2. Want to change from PWM to SMB?
3. Want to start reading in PWM?
4. Want to start reading in SMBUS?

### Want to change from SMB to PWM?

This assumes the MLX90614 is already in SMB mode and connected to the
I2C channel on the Arduino. It also expects that the VCC wire from the
MLX90614 is connected to the defined PWR_PIN.

The following parameters (which can be set in the top of the sketch)
will be used:

PWM_OBJ_TEMP
<br>When set as true the PWM signal will provide the object temperature. When
set as false the Ambient temperature.

p.s. Although it is possible to provide BOTH the Ambient and Object
temperature in extended PWM mode, the timing  dependency becomes even
more critical and also the complexity. I kept it simple.

PWM_TEMP_MIN
<br>The PWM signal width will be relative the range (between minimum and
maximum temperature). The smaller the range the more accurate the
reading of the PWM signal. With this parameter you can set the minimum
temperature. The value is in Celsius and the lowest allowed is -20C

PWM_TEMP_MAX
<br>With this parameter you can set the maximum temperature. The value is
in Celsius and the highest allowed is 120C.

PWM_CLOCK
<br>You can adjust the frequency of the PWM signal. When set as '2' the
frequency is 1Khz, '4' will give you 500Hz etc.

REMEMBER REMEMBER REMEMBER REMEMBER REMEMBER REMEMBER

Make sure to write down setting for PWM_OBJ_TEMP, PWM_TEMP_MIN and
PWM_TEMP_MAX as you need this information when reading in PWM mode.

REMEMBER REMEMBER REMEMBER REMEMBER REMEMBER REMEMBER

The different registers are updated and a reboot is performed. After
reboot PWM reading is started. MAKE SURE TO CONNECT THE SDA-LINE to
the defined PWM_PIN.

### Want to change from PWM to SMB?

This assumes the MLX90614 is already in PWM mode and connected to the
I2C channel on the Arduino. IT MUST BE the ONLY sensor on the I2C
channel. It also expects that the VCC wire from the MLX90614 is
connected to the defined PWR_PIN.

This sets a pulse on SCL to interrupt PWM mode, then the PWM CTRL
register is updated to set SMBus mode and performs a reboot. The reading
is started in SMBus mode.
The output is provided as Celsius, Fahrenheit or Kelvin, depending on
the defined SMB_TEMP_UNIT.

### Want to start reading in PWM?

MAKE SURE TO CONNECT THE SDA-LINE to the defined PWM_PIN.
The PWM signal will be interpreted based on the defined variables
PWM_OBJ_TEMP, PWM_TEMP_MIN and PWM_TEMP_MAX. It will display the
temperature in Celsius and Fahrenheit.

### Want to start reading in SMB?

MAKE SURE TO CONNECT THE MLX90614 is connected to the I2C channel.
The output is provided as Celsius, Fahrenheit or Kelvin, depending on
the defined SMB_TEMP_UNIT.

## reading PWM

Once the MLX90614 has been set succesfully in PWM mode, you can now
read it on nearly any board. A sketch 'mlx_pwm_read' is provided.
I have tested on Arduino UNO, MEGA250, Artemis ATP with the V1 library
AND the V2 library.

You can power use a pin or directly to the 3V3 (in the later case set
PWR_PIN as zero)

MAKE SURE TO CONNECT THE SDA-LINE to the defined PWM_PIN. This pin
needs to be able to raise an interrupt. On an Arduino there are only a
few. see https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
On Artemis / Apollo3 all pins are able to raise an interrupt.

The PWM signal will be interpreted based on the defined variables
PWM_OBJ_TEMP, PWM_TEMP_MIN and PWM_TEMP_MAX. It will display the
temperature in Celsius and Fahrenheit.

## Installation
For the sketch 'mlx_change_mode 'make sure you have the Sparkfun library for the
MLX90614 installed. http://librarymanager/All#Qwiic_IR_Thermometer by SparkFun

For the sketch 'mlx_pwm_read', you do NOT need this.

The following sketches are provided

* mlx_change_mode   : to switch between PWM and SMBuS
* mlx_pwm_read      : to read the MLX90614 in PWM mode
* mlx_ReadRegisters : to read the MLX90614 registers in SMBus mode

Version 1.0 / November 2021 / paulvha
