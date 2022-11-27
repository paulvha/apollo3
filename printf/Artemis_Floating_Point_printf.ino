/**
 * This sketch will demonstrate the issue (and solution) for printing floating value
 *
 * The problem:
 * In the Apollo3 V2 library the printf function is embedded from Mbed. As part of creating the pre-compiled Mbed-archive
 * only a limited printf is included. This limited function has a number of issues
 */

void setup() {

  char Sbuf[20];

  Serial.begin(115200);
  delay(1000);
  Serial.println("Test floating point display");

  // example
  float a = 5.02131234;

  // the standard NONE changed version will provide
  // %2.4f with this call
  sprintf(Sbuf,"%2.4f",a);
  Serial.printf("With standard (s)printf: %s\n", Sbuf);

  // this is the relying on the patched version of HardwareSerial.cpp
  // see the install.txt and this will show: 5.0213
  Serial.printf("With patched printf %2.4f\n", a);

  // this is the standard  dtostrf(), but that is using sprintf
  // Artemis_dtostrf() is using Artemis_sprintf it will show: 5.0213
  Serial.printf("Using Artemis_dtostrf %s", Artemis_dtostrf(a,2,4,Sbuf));
}

void loop() {
  // put your main code here, to run repeatedly:

}
