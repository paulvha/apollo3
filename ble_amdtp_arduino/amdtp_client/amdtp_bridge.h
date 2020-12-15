#ifndef _BLE_AMDTP_H_
#define _BLE_AMDTP_H_

#include <ArduinoBLE.h>
#include <Arduino.h>

//*****************************************************************************
//
// Server commands
//
//*****************************************************************************
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


#define AMD_IDLE      0
#define AMD_SCANNED   1
#define AMD_CONNECTED 2

//*****************************************************************************
//
// Forward declarations.
//
//*****************************************************************************
void RxChar_Received(BLEDevice central, BLECharacteristic characteristic);
void TxChar_Received(BLEDevice central, BLECharacteristic characteristic);
void AckChar_Received(BLEDevice central, BLECharacteristic characteristic);
void TxChar_Write(uint8_t *value, uint16_t vlen);
void AckChar_Write(uint8_t *value, uint16_t vlen);
void encode_receipt(String a, int vsize);
void PrepSending(uint8_t *value, uint16_t vlen);

// for client
bool StartBLE();
bool get_handles();
bool PerformScan();
bool CheckConnect();
void perform_disconnect();
void HandleServerResp(uint8_t * buf, uint16_t len);

extern void set_led_high( void );
extern void set_led_low( void );

#endif // _BLE_AMDTP_H_
