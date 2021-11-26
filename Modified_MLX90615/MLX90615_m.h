/**
 * this is a modified version of the original Groove/Seed library
 * removed all reference to the I2C soft Master
 *
 * included support for PWM measurements
 *
 * version 1.0 */

#ifndef __MLX90615_M_H__
#define __MLX90615_M_H__

#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <stdbool.h>

#define MLX90615_EEPROM_SA          0x10
#define MLX90615_EEPROM_PWMT_MIN    MLX90615_EEPROM_SA
#define MLX90615_EEPROM_PWMT_RNG    0x11
#define MLX90615_EEPROM_CONFIG      0x12
#define MLX90615_EEPROM_EMISSIVITY  0x13

#define MLX90615_RAW_IR_DATA            0x25
#define MLX90615_AMBIENT_TEMPERATURE    0x26
#define MLX90615_OBJECT_TEMPERATURE     0x27

#define MLX90615_SLEEP	0xC6

// DEPRECATED! (just emissivity, not the whole EEPROM)
#define AccessEEPROM                    MLX90615_EEPROM_EMISSIVITY

#define Default_Emissivity              0x4000
#define MLX90615_DefaultAddr			0x5B

// DEPRECATED! (too ambiguous in some setups)
#define DEVICE_ADDR                     MLX90615_DefaultAddr

// prototypes for PWM
void SetInstance(void *inst);
void _PWM_isr();

class MLX90615 {

  protected:
    TwoWire* wbus;

    union {
        uint8_t	buffer[5];
        struct {
            union {
                uint8_t dev;
                struct {
                    uint8_t             : 1;
                    uint8_t i2c_addr    : 7;
                };
            };
            uint8_t cmd;
            uint8_t dataLow;
            uint8_t dataHigh;
            uint8_t pec;
        };
    };

    uint8_t _PWM_pin;
    unsigned long _PWM_HighStartTime;
	unsigned long _PWM_LowStartTime;
	unsigned long _PWM_Period[2];
    unsigned long _PWM_High[2];
	bool _PWM_validate[2];
	uint8_t _PWM_val = 0;
	bool _CalcDutyCycle = false;

  public:

    /*******************************************************************
        Function Name: init
        Description:  initialize for i2c device.
        Parameters: sda pin, scl pin, i2c device address
        Return: null
    ******************************************************************/

    MLX90615(uint8_t addr, TwoWire* i2c) {
        dev = addr << 1;
        wbus = i2c;
    }

    /****************************************************************
        Function Name: crc8_msb
        Description:  CRC8 check to compare PEC data
        Parameters: poly - x8+x2+x1+1, data - array to check, array size
        Return: 0 – data right; 1 – data Error
    ****************************************************************/
    uint8_t crc8Msb(uint8_t poly, uint8_t* data, int size)	{
        uint8_t crc = 0x00;
        int bit;

        while (size--) {
            crc ^= *data++;
            for (bit = 0; bit < 8; bit++) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ poly;
                } else {
                    crc <<= 1;
                }
            }
        }

        return crc;
    }

    /*******************************************************************
        Function Name: isConnected
        Description:  check for device on address
        Parameters:
        Return: true if detected else false
    ******************************************************************/
    bool isConnected()
    {
        wbus->beginTransmission(i2c_addr);
        if (wbus->endTransmission() == 0)
            return true;
        return false;
    }


    /*******************************************************************
        Function Name: pwm_init
        Description:  intialise PWM for reading
        Parameters: pin on which to expect the PWM data
        Return: None
    ******************************************************************/
    void pwm_init(uint8_t ComPin)
	{
		_PWM_pin = ComPin;

        pinMode(_PWM_pin,INPUT);

        // init received values
		for(uint8_t i =0; i < 2;i++) {
			_PWM_Period[i] = _PWM_High[i] = 0;
			_PWM_validate[i] = false;
		}

        _PWM_HighStartTime = _PWM_LowStartTime = 0;

        // Point to new instance's ISR routine
        SetInstance(this);

        attachInterrupt(digitalPinToInterrupt(_PWM_pin), _PWM_isr,  CHANGE);
	}

    /*******************************************************************
        Function Name: getPWMTemperature
        Description:  Calculate and return the temperature from PWM
        Parameters:
        bool: true for Fahrenheit scale, false (or unspec) for Celsius scale
        Return: Measured temperature or zero if invalid
    ******************************************************************/
	float getPWMTemperature(bool fahrenheit = false)
	{
		float pwm_period, pwm_high, duty;
        uint8_t off = _PWM_val ? 0 : 1;

        // if no valid data yet
        if(! _PWM_validate[off]) return (0);

		// prevent data overwrite
		_CalcDutyCycle = true;

        pwm_period = _PWM_Period[off];
        pwm_high = _PWM_High[off];

        // enable data capture
		_CalcDutyCycle = false;

		duty = (pwm_high - (pwm_period / 8)) / pwm_period;

		float t = (2 * duty) * (50 - 0)  + 0;

        return fahrenheit ? (t * 1.8) + 32.0 : t;
	}

    /*******************************************************************
        Function Name: rxBit
        Description: handles when input line level changes
        Parameters:
        Return: none
    ******************************************************************/
    void rxBit(void)
    {
	  // Capture current time
	  unsigned long bitTime = micros();

      // if high
      if (digitalRead(_PWM_pin)) {

        if (_PWM_HighStartTime == 0) {

            _PWM_HighStartTime = bitTime;
            _PWM_LowStartTime = 0;

        }
        else if (_PWM_HighStartTime != 0 && _PWM_LowStartTime != 0) {

            // do not move forward when calculating duty cycle
            if ( !_CalcDutyCycle ) {

                // check for a time overrun / turn around after many days power-on
                if (bitTime > _PWM_HighStartTime)
                {
                    _PWM_Period[_PWM_val] = bitTime - _PWM_HighStartTime;
                    _PWM_High[_PWM_val] = _PWM_LowStartTime - _PWM_HighStartTime;

                    // indicate as complete
                    _PWM_validate[_PWM_val] = true;

                    // keep 2 different values
                    _PWM_val++;
                    if(_PWM_val > 1) _PWM_val = 0;
               }
            }

            // re-init
            _PWM_HighStartTime = 0;
            _PWM_LowStartTime = 0;
            _PWM_validate[_PWM_val] = false;
          }
      }
      else { // low
          if (_PWM_HighStartTime != 0) {
              _PWM_LowStartTime = bitTime;
          }
       }
    }

    /******************************************************************
        Get temperature in Celcius or Fahrenheit in SMB mode
        Parameters:
        > Temperature_kind: MLX90615_AMBIENT_TEMPERATURE or MLX90615_OBJECT_TEMPERATURE
        > bool: true for Fahrenheit scale, false (or unspec) for Celsius scale
        Return:  Temperature
    ******************************************************************/
    float getTemperature(int Temperature_kind, bool fahrenheit = false) {
        float celsius;
        uint16_t tempData;

        readReg(Temperature_kind, &tempData);

        double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)

        // This masks off the error bit of the high byte
        celsius = ((float)tempData * tempFactor) - 0.01;

        celsius = (float)(celsius - 273.15);

        return fahrenheit ? (celsius * 1.8) + 32.0 : celsius;
    }

    /****************************************************************
        Read a MLX90615 register.
        @param MLXaddr: MLX90615 EEPROM/RAM address
        @param result: Pointer to variable to store the readed value
        @return: status:   0  OK
                         -1  Bad CRC calc
                         -2  I2C Error
                        -10  I2C Connector not specified yet
    ****************************************************************/
    int readReg(uint8_t MLXaddr, uint16_t* resultReg) {
        if (wbus) {
            // Using Wire
            wbus->beginTransmission(i2c_addr);
            wbus->write(MLXaddr);
            wbus->endTransmission(false);
            if (wbus->requestFrom(i2c_addr, (uint8_t)3) == 3) {
                dataLow = wbus->read();
                dataHigh = wbus->read();
                pec = wbus->read();
                *resultReg = dataHigh << 8 | dataLow;
                return 0;
            } else {
                return -2;
            }
        } else {
            return -10;
        }
    }

    /**************************************************************
        Write a MLX90615 register. If its an EEPROM register,
        please call twice: first with 0x0000, second with desired value
        @param MLXaddr: MLX90615 EEPROM/RAM address
        @param value: ... to be writen
        @return: status:   0  OK
                         -1  Bad CRC calc
                         -2  I2C Error
                        -10  I2C Connector not specified yet
    ***************************************************************/
    int writeReg(uint8_t MLXaddr, uint16_t value) {
        // CRC calculation
        cmd = MLXaddr;
        dataLow = value & 0xff;
        dataHigh = (value >> 8) & 0xff;
        pec = crc8Msb(0x07, buffer, 4);
        if (crc8Msb(0x07, buffer, 5)) {
            return -1;
        }

        int sent = 0;
        if (wbus) {
            // Using Wire
            wbus->beginTransmission(i2c_addr);
            sent = wbus->write(&buffer[1], 4);
            wbus->endTransmission();
            if (sent != 4) {
                return -2;
            }
            return 0;
        } else {
            return -10;
        }
    }
};

// needed for callback from interrupt
MLX90615 *active_pwm_handle = 0;

/*******************************************************************
    Function Name: SetInstance
    Description:  Set instance to call back
    Parameters: pointer to instance (this)
    Return: none
******************************************************************/
void SetInstance(void *inst)
{
    active_pwm_handle = (MLX90615 *) inst;
}

/*******************************************************************
    Function Name: _PWM_isr
    Description:  call back when interrupt happens
    Parameters:
    Return: none
******************************************************************/
void _PWM_isr()
{
  active_pwm_handle->rxBit();
}


#endif // __MLX90615_M_H__
