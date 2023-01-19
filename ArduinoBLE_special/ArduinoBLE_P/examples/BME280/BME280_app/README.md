# Android-BLE-BME280

This app will search for a peripheral with the name "Peripheral BME280 BLE". This is the default name of the Arduino_P peripheral example sketch "example20_ph_BME280". peripheral is what Androiders would call it, a server.

<br> Once detetected it will connect and scan for the Service and the characteristics. It will subscribe to the notify messages and wait for the peripheral to send updated BME280 data.

It will then display the measured Temperature, humidity, pressure and Altitude.

From the App you can use the buttons to send request to the peripheral to
<br> "Get now" where the pheriperal will send the data immediately
<br> receive the temperature in Fahrenheit or Celsius
<br> receive the alitude in meters or feet.

Once you are done, you can use the disconnect_button.

Version 1.0 / January 2023 / Paulvha@hotmail.com
* initial version
