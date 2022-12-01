/*
 * use nRFConnect on your mobile
 *
 * scan and look for device starting with 'PVH
 * No need to connect
 *
 * The name will change every 5 seconds and show the ADC values
 *
 *
 * version 1.0 / paulvha / November 2022
 */

#include <ArduinoBLE.h>

int valADC1 = 0;
int valADC2 = 0;
int valADC3 = 0;

// define the pins
#define ADC1 A0
#define ADC2 A1
#define ADC3 A2

void setup()
{
  Serial.begin(115200);    // initialize serial communication
  while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!. Freeze");
    while (1);
  }
}

void loop()
{
  char buf[20];

  // obtain the values
  valADC1 = analogRead(ADC1);
  valADC2 = analogRead(ADC2);
  valADC3 = analogRead(ADC3);

  // turn to name
  sprintf(buf,"PVH 1:%d, 2:%d, 3:%d",valADC1, valADC2, valADC3);

  BLE.stopAdvertise();
  BLE.setLocalName(buf);
  BLE.advertise();

  Serial.println("update");

  delay(5000);
}
