#Output_Input_Control

This example consists of 2 parts
	Sketch for Arduino to work as peripheral
	Andoid app that can connect to the peripheral and control input and output

In the Ubuntu section of the ArduinoBLE_P there is also an example (output_input_control) that
can be compiled in to interact from Ubuntu with the sketch

## Perirpheral sketch Output / input _ control

  Version 1.0 / December 2022 / paulvha
  * initial version

  This example creates a Bluetooth® Low Energy peripheral with service that contains
  A characteristic to reveive commands to control :
    The onboard LED
    Set output high / low on 2 different pins
    Get input from one digital & one analog pin.
    Request simulated battery level

  On the notify characteristic it will send :
    Status input from one digital & one analog pin.
    Simulated battery level every x minutes

  To those output pins you can connect a relay or other LEDs.

  It is an extension to the default / standard LED peripheral.

===============================================================================

## Android-ble-led-relay APP

Example application that allows you to scan, and connect to a ble device on Android (M) API 23

After connecting to a peripheral Arduino Sketch 'Output_Input_control', it will present buttons. You can either request
<br> setting the Arduino Onboard led on/off
<br> switch 2 output pins between HIGH / LOW
<br> get the input from a digital pin
<br> get the input from an Analog pin
<br> ask for regular update of simulated battery level

version 1.0 / December 2022 / paulvha

