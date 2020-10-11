#ifndef _BLE_AMDTP_H_
#define _BLE_AMDTP_H_

#include <ArduinoBLE.h>
#include <Arduino.h>

// command to sent between client and server
// make sure to stay in sync with the list on the client !!!
typedef enum eAmdtpcmd
{
    AMDTP_CMD_NONE,
    AMDTP_CMD_START_TEST_DATA,
    AMDTP_CMD_STOP_TEST_DATA,
    AMDTP_CMD_HELLO,
    AMDTP_CMD_REQ_BATTERY_LEVEL,
    AMDTP_CMD_REQ_BATTERYLOAD_ON,
    AMDTP_CMD_REQ_BATTERYLOAD_OFF,
    AMDTP_CMD_REQ_INTERNAL_TEMP_CEL,
    AMDTP_CMD_REQ_INTERNAL_TEMP_FRH,
    AMDTP_CMD_TURN_LED_ON,
    AMDTP_CMD_TURN_LED_OFF,
    AMDTP_CMD_BME280,
    AMDTP_CMD_ADC,
    AMDTP_CMD_CHAT,
    AMDTP_CMD_READ_PIN,
    AMDTP_CMD_PIN_HIGH,
    AMDTP_CMD_PIN_LOW,
    AMDTP_CMD_VERSION,
    AMDTP_CMD_CUSTOM1,
    AMDTP_CMD_CUSTOM2,
    AMDTP_CMD_CUSTOM3,
    AMDTP_CMD_CUSTOM4,
    AMDTP_CMD_CUSTOM5,
    AMDTP_CMD_MAX
}eAmdtpPktcmd_t;


/* needed for conversion float IEE754 */
typedef union {
    byte array[4];
    float value;
} ByteToFloat;

//*****************************************************************************
//
// Forward declarations.
//
//*****************************************************************************
void blePeripheralConnectHandler(BLEDevice central);
void blePeripheralDisconnectHandler(BLEDevice central);
void RxChar_Received(BLEDevice central, BLECharacteristic characteristic);
void TxChar_Received(BLEDevice central, BLECharacteristic characteristic);
void AckChar_Received(BLEDevice central, BLECharacteristic characteristic);
void TxChar_Write(uint8_t *value, uint16_t vlen);
void AckChar_Write(uint8_t *value, uint16_t vlen);
void UserRequestReceived(uint8_t * buf, uint16_t len);
void encode_receipt(String a, int vsize);
void PrepSending(uint8_t *value, uint16_t vlen);

extern void set_led_high( void );
extern void set_led_low( void );
extern BLEStringCharacteristic TxChar;
extern BLEStringCharacteristic AckChar;


#endif // _BLE_AMDTP_H_
