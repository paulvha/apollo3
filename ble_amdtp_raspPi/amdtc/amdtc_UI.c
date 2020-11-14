// ****************************************************************************
//
//
//! @file  amdtp_UI.c
//!
//! @brief This file provides the client user interface for the amdtc service
//!
//! This has been created and tested to work against Apollo3-board running ble_amdts.ino
//!
//! paulvha / February 2020 / version 1.0

//! paulvha/ March 2020 / version 2.0
/*
 * update ADC pin validation (now on server)
 * update Celius, Fahrenheit and battery level to float values
 * added major/minor server and client number
 *
 */
//! @{
//
// ****************************************************************************
//*****************************************************************************
//
// Copyright (c) 2020, Paul van Haastrecht
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include <stdbool.h>
#include <glib.h>

#include "lib/sdp.h"
#include "lib/uuid.h"
#include "btio/btio.h"
#include "gattrib.h"

#include "amdtc_UI.h"
#include "amdtc.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
struct termios orig_termios;        // save in case of test data
gboolean ConioSet = false;          // indicate that termios was saved

gboolean b_load = false;            // remember whether battery load resistor was set
gboolean SaveInteractive;           // used when resistor was set to restore
extern uint8_t g_debug;             // Debug level from command line
uint8_t GetValue = AMDTP_CMD_NONE;  // which value to get next

// boolean (like op_tempC) are defined in amdtc.c
// as they can be set on the command line as well as interactive
extern gboolean opt_TempC;
extern gboolean opt_TempF;
extern gboolean opt_Batt;
extern gboolean opt_Batt_load;
extern gboolean opt_TurnOn;
extern gboolean opt_TurnOff;
extern gboolean opt_interactive;
extern gboolean opt_bme280;
extern gboolean opt_read_pin;
extern gboolean opt_pin_high;
extern gboolean opt_pin_low;
extern gboolean opt_adc;
extern gboolean opt_quiet;
extern int opt_adc_ch;              // ADC channel to read
extern int opt_pin_rd;              // Pin to read
extern int opt_high_pin;            // Pin to set high
extern int opt_low_pin;             // Pin to set low

// not defined external as they can only be set interactive
gboolean opt_start_data = FALSE;
gboolean opt_stop_data = FALSE;
gboolean opt_chat = FALSE;
gboolean FirstChat;
gboolean opt_vers = FALSE;


/**
 * check for valid digital pin
 *
 * Return : TRUE for OK else FALSE
 */
gboolean validate_pin(int pin)
{
    if(pin >= 1 && pin <= 49) {
        if(pin != 30 && pin != 46) return TRUE;
    }

    return FALSE;
}

/**
 * read interactive digital pin to use
 * @param cmd : the command for which the pin is mend
 *
 * @return : digital pin or zero in case of error
 */
uint8_t get_Digital_Pin(int cmd)
{
    int ret, selection;
    gboolean found = FALSE;

    switch(cmd) {
        case AMDTP_CMD_READ_PIN:
            selection = opt_pin_rd;
            opt_pin_rd = 0;
            break;
        case AMDTP_CMD_PIN_HIGH:
            selection = opt_pin_high;
            opt_pin_high = 0;
            break;
        case AMDTP_CMD_PIN_LOW:
            selection = opt_pin_low;
            opt_pin_low = 0;
            break;
        default:
            return 0;
    }
    // if channel provided on the command line
    if (selection) return selection;

    // just in case we are receiving test data
    if (opt_start_data)  rst_terminal();

    do{
        g_print("Enter Digital pin : (0 = cancel)");
        ret = scanf("%d", &selection);

        if (selection == 0) {
            g_print("Cancel selection\n");
            found = TRUE;
        }
        else {
            found = validate_pin(selection);
        }
    } while(found == FALSE);

    if (opt_start_data)  set_terminal();

    return((uint8_t) selection);
}

/**
 * obtain a ADC channel to read from user
 * validation is done on the server, given that different boards
 * use different pins
 */
uint8_t get_ADC_Channel()
{
    int ret, selection;

    // if channel provided on the command line
    if (opt_adc_ch) {
        selection = opt_adc_ch;
        opt_adc_ch = 0;
        return selection;
    }

    // just in case we are receiving test data
    if (opt_start_data)  rst_terminal();

    g_print("Enter ADC pin (0 = cancel): ");
    ret = scanf("%d", &selection);

    if (selection == 0) {
        g_print("Cancel selection\n");
    }

    if (opt_start_data)  set_terminal();

    return((uint8_t) selection);
}

/**
 * chat with server
 *
 * get a line from the client and sent to the server
 *
 * Chat is terminated by typing "BYE". It will return FALSE in that case
 */
gboolean chat(char *buf)
{
   int len = 0;
   char c;

   g_print("\n>>>>> ");

   while(1) {

        if (scanf("%c",&c) == 1){

            // no empty line, else done
            if (c == 0xa && len > 0) {
                buf[len++] = 0x0;
                break;
            }

            // no CR or NL
            if (c != 0xd && c != 0xa) buf[len++] = c;

            // buffer full ?
            if (len > MAX_BUF - 5){
                buf[len++] = 0x0;
                break;
            }
        }
    }

    // check for stopping chat
    if (len == 4)
        if (strcmp(buf,"BYE") == 0) return FALSE;

    return TRUE;
}

/**
 * @brief In this MainLoop data will be exchanged.
 *
 * THis will be called from commACK_cb() as we have received valid
 * data from the server and have completed sending an ACK on that.
 *
 * The commands can also be set from command line
 *
 * buf [0] = request that was sent
 * buf [1] = length of additional data
 * buf [2] ..... additional data
 *
 * This client will take the initiative
 *
 * The case-options (like AMDTP_CMD_HELLO) must align and are defined in amdtc_UI.h
 */
void MainLoop(uint8_t *buf, uint16_t len)
{
    int16_t val, i;
    gboolean ret;
    float fret;

    if (g_debug > 0) {
        g_print("UI %d bytes data received : ", len);
        for (i = 0; i < len; i++)  g_print("0x%x ", buf[i]);
        g_print("\n");
    }

    // handle / display response received first
     switch (buf[0]) {

        case AMDTP_CMD_HELLO:
            if (opt_quiet == FALSE) g_print("\nReceived HELLO responds\n");
            break;

        case AMDTP_CMD_TURN_LED_ON:
            if (opt_quiet == FALSE) g_print("\nTurned Led on\n");
            break;

        case AMDTP_CMD_TURN_LED_OFF:
            if (opt_quiet == FALSE) g_print("\nTurned Led off\n");
            break;

        case AMDTP_CMD_REQ_BATTERYLOAD_ON:
            if (opt_quiet == FALSE) g_print("\nTurned load resistor ON\n");
            b_load = TRUE;  // resistor was set
            break;

        case AMDTP_CMD_REQ_BATTERYLOAD_OFF:
            if (opt_quiet == FALSE) g_print("\nTurned load resistor OFF\n");
            break;

        case AMDTP_CMD_REQ_BATTERY_LEVEL:
            if (g_debug >0) g_print("\nRead internal battery level\n");
            fret = byte_to_float(buf, 2);
            if (opt_quiet) g_print("%2.2f %%\n", fret);
            else g_print("\nBattery level is at %2.2f %%\n", fret);
            break;

        case AMDTP_CMD_REQ_INTERNAL_TEMP_FRH:
            if (g_debug >0) g_print("\nRead internal temperature in Fahrenheit\n");
            fret = byte_to_float(buf, 2);
            if (opt_quiet) g_print("%2.2f %% F\n", fret);
            else g_print("\nInternal temperature is %2.2fF\n", fret);
            break;

        case AMDTP_CMD_REQ_INTERNAL_TEMP_CEL:
            if (g_debug >0) g_print("\nRead internal temperature in Celsius\n");

            fret = byte_to_float(buf, 2);
            if (opt_quiet) g_print("%2.1f %% C\n", fret);
            else g_print("\nInternal temperature is %2.1fC\n", fret);
            break;

        case AMDTP_CMD_BME280:
            if (g_debug >0) g_print("\nRead BME280\n");

            display_BME280(buf, len);
            break;

        case AMDTP_CMD_START_TEST_DATA:
            g_print("\nStart test data\n");

            val = buf[len-2] << 8 | buf[len - 1];
            g_print("\n%s %d\n", &buf[2], val);
            break;

        case AMDTP_CMD_STOP_TEST_DATA:
            g_print("\nStopped test data\n");
            break;

        case AMDTP_CMD_ADC:
            if (g_debug > 0) g_print("\nRead ADC channel\n");

            val = buf[3] << 8 | buf[4];
            if (val == -1){
                g_print("%d : INCORRECT analogPin\n", buf[2]);
            }
            else
            {
                if (opt_quiet) g_print("%2.1f\n", (float) val);
                else
                 g_print("\nADC value channel %d is %2.1f or %2.1f volt\n", buf[2], (float) val, (float)((float)val * 3.3 / 1023));
            }
            break;

        case AMDTP_CMD_CHAT:
            if (g_debug > 0) g_print("\nChat received\n");

            g_print("<<<<< %s\n", &buf[2]);

            // if receiving BYE stop chatting
            if (len == 6){
                if (strcmp((char *) &buf[2],"BYE") == 0) opt_chat = FALSE;
            }
            break;

        case AMDTP_CMD_READ_PIN:
            if (g_debug > 0) g_print("\nRead pin value\n");

            if (opt_quiet) g_print("%s\n", buf[3]?"HIGH":"LOW");
            else g_print("\nDigital pin %d is reading %s\n", buf[2], buf[3]?"HIGH":"LOW");
            break;

        case AMDTP_CMD_PIN_HIGH:
            if (g_debug > 0) g_print("\nSet pin high\n");

            if (opt_quiet == FALSE) g_print("\nDigital pin %d is set high\n", buf[2]);
            break;

        case AMDTP_CMD_PIN_LOW:
            if (g_debug > 0) g_print("\nSet pin low\n");

            if (opt_quiet == FALSE) g_print("\nDigital pin %d is set low\n", buf[2]);
            break;

        case AMDTP_CMD_VERSION:
            if (buf[2] != MAJOR_CLIENTVERSION){
               g_print("*****************************************\n");
               g_print("** WARNING MAJOR VERSIONS DO NOT MATCH **\n");
               g_print("**    not all features supported !!    **\n");
               g_print("*****************************************\n");
            }
            g_print("\nServer version: %d.%d\n", buf[2], buf[3]);
            g_print("Client version: %d.%d\n",MAJOR_CLIENTVERSION, MINOR_CLIENTVERSION);

            break;
        case AMDTP_CMD_CUSTOM1:      // repeat for other options
             if (g_debug >0) g_print("\nAMDTP_CMD_CUSTOM1\n");

            // Do something here....
              break;

         default:
            if (g_debug >0)
                g_print("\nUNKNOWN request/ answer returned : 0x%x\n",buf[0]);
            break;
    }

retry:

    // NEXT.. check and perform any pending / command line provided commands
    DetermineValue(TRUE);

    // sending test data needs to be stopped by keyboard input
    // requesting test data is NOT a command line option only interactive
    if (GetValue == AMDTP_CMD_NONE || opt_start_data) {

        // If interactive was requested
        if (opt_interactive) read_interactive();

        DetermineValue(TRUE);
    }

    if (GetValue == AMDTP_CMD_NONE)  goto done;

    // following commands need additional data to be included in the message

    if (GetValue == AMDTP_CMD_ADC) {
        uint8_t ADC_channel = get_ADC_Channel();
        if (ADC_channel == 0) goto retry;
        ret = characteristics_write_req(GetValue, &ADC_channel, 1);
    }
    else if (GetValue == AMDTP_CMD_READ_PIN || GetValue == AMDTP_CMD_PIN_HIGH || GetValue == AMDTP_CMD_PIN_LOW) {
        uint8_t Pin = get_Digital_Pin(GetValue);
        if (Pin == 0) goto retry;
        ret = characteristics_write_req(GetValue, &Pin, 1);
    }
    else if (GetValue == AMDTP_CMD_CHAT){
        char buf[MAX_BUF];
        if (FirstChat) g_print ("Enter: BYE to terminate chat\n");
        opt_chat = chat(buf);
        if (opt_chat == FALSE && FirstChat == TRUE) goto retry;
        FirstChat = FALSE;
        // length + 1 to add the 0x0 terminator
        ret = characteristics_write_req(GetValue, buf,strlen(buf)+1);
    }
    else {
        // write request
        ret = characteristics_write_req(GetValue, NULL,0);
    }

    if (ret == TRUE) return;
    got_error = TRUE;
done:
    g_main_loop_quit(event_loop);
}

/**
 * @brief : translate 4 bytes to float IEEE754
 * @param x : offset in received data
 *
 * return : float number
 */
float byte_to_float(uint8_t *buf, int x)
{
    ByteToFloat conv;
    uint8_t i;
    for (i = 0; i < 4; i++)  conv.array[3-i] = buf[x+i];
    return conv.value;
}

/**
 * @brief display BME280.
 *
 * The buffer format is:
 *
 * 1 byte  original request
 * 1 byte  len of data (it 0 BME280 not enabled or NOT detected on server else 18)
 * 4 bytes float humidity
 * 4 bytes float pressure
 * 1 byte  indicator : M (meter) or F (feet). This is determined in sketch.
 * 4 bytes float altitude
 * 1 byte  indicator : C (Celsius) or F (Fahrenheit). This is determined in sketch.
 * 4 bytes float temperature
 */
void display_BME280(uint8_t *buf, uint16_t len)
{
    float ret;
    uint8_t x = 2 ;     // skip orginal request + len
    char ind;

    if (buf[1] == 0 ) {
        g_print("BME280 was not detected or not implemented on server\n");
        return;
    }

    if (buf[1] != 18 ) {
        g_print("Not enough data received. Expected 18 bytes, got %d\n", buf[1]);
        return;
    }

    // get temperature
    ind = buf[x++];
    ret = byte_to_float(buf, x);
    g_print("\nTemperature\t%2.2f %c\n", ret, ind);
    x += 4;

    // get humidity
    ret = byte_to_float(buf, x);
    g_print("Humidity\t%2.2f %%\n", ret);
    x += 4;

     // get pressure
    ret = byte_to_float(buf, x);
    g_print("Pressure\t%2.2f hPa\n", ret/100);
    x += 4;

    // get altitude
    ind = buf[x++];
    ret = byte_to_float(buf, x);
    g_print("Altitude\t%2.2f %c\n", ret, ind);
}

/**
 * @brief Check whether there is a request to sent to server
 * @param get : if true remove the option and return value,
 *              if false only return the value (a check)
 *
 * NOTE:
 * The GetValue-options must align and are defined in amdtc_UI.h
 *
 * The boolean (like op_tempC) are defined in amdtc.c and indicated
 * as external in top of this program.
 */
void DetermineValue(gboolean get)
{
    GetValue = AMDTP_CMD_NONE;

    if (opt_TempC) {
        if (get) opt_TempC = FALSE;
        GetValue = AMDTP_CMD_REQ_INTERNAL_TEMP_CEL;
    }
    else if (opt_TempF) {
        if (get) opt_TempF = FALSE;
        GetValue = AMDTP_CMD_REQ_INTERNAL_TEMP_FRH;
    }
    else if (opt_Batt_load) {           // read battery level with load resistor
        if (get) opt_Batt_load = FALSE; // remove flag

        if (b_load) {                   // if load was set
            b_load = FALSE;
            opt_interactive = SaveInteractive;         // restore the original interactive
            GetValue = AMDTP_CMD_REQ_BATTERYLOAD_OFF;  // reset battery load
        }
        else {
            opt_Batt = TRUE;       // enable battery level reading always after battery resistor setting
            SaveInteractive = opt_interactive;        // save the option value;
            opt_interactive = FALSE;                  // reset for now
            GetValue = AMDTP_CMD_REQ_BATTERYLOAD_ON;  // set battery load
        }
    }
    else if (opt_Batt)  {
        if (get) opt_Batt = FALSE;

        if (b_load) {               // if resistor was set we need to remove after this battery level read
            opt_Batt_load = TRUE;
        }
        GetValue = AMDTP_CMD_REQ_BATTERY_LEVEL;  // read the battery level
    }
    else if (opt_TurnOn) {
        if (get) opt_TurnOn = FALSE;
        GetValue = AMDTP_CMD_TURN_LED_ON;
    }
    else if (opt_TurnOff) {
        if (get) opt_TurnOff = FALSE;
        GetValue = AMDTP_CMD_TURN_LED_OFF;
    }
    else if(opt_start_data) {    // option is reset in read_interactive()
        GetValue = AMDTP_CMD_START_TEST_DATA;
    }
    else if(opt_stop_data) {
        opt_stop_data = FALSE;
        GetValue = AMDTP_CMD_STOP_TEST_DATA;
    }
    else if(opt_bme280) {
        if (get) opt_bme280 = FALSE;
        GetValue = AMDTP_CMD_BME280;
    }
    else if(opt_adc) {
        if (get) opt_adc = FALSE;
        GetValue = AMDTP_CMD_ADC;
    }
    else if(opt_read_pin) {
        if (get) opt_read_pin = FALSE;
        GetValue = AMDTP_CMD_READ_PIN;
    }
    else if(opt_pin_high) {
        if (get) opt_pin_high = FALSE;
        GetValue = AMDTP_CMD_PIN_HIGH;
    }
    else if(opt_pin_low) {
        if (get) opt_pin_low = FALSE;
        GetValue = AMDTP_CMD_PIN_LOW;
    }
    else if(opt_chat) {           // option is reset in chat session
        GetValue = AMDTP_CMD_CHAT;
    }
    else if(opt_vers) {
        if (get) opt_vers = FALSE;
        GetValue = AMDTP_CMD_VERSION;
    }
}

/**
 * @brief Display the options
 *
 * Add a line for each new option.
 */
void display_menu()
{
    int sel = 0;

    g_print("\nMake your selection :\n");
    g_print("%d\tQuit\n", sel++);
    g_print("%d\tStart receiving test data\n",sel++);
    g_print("%d\tStop receiving test data\n",sel++);
    g_print("%d\tRead internal temperature in Celsius\n",sel++);
    g_print("%d\tRead internal temperature in Fahrenheit\n",sel++);
    g_print("%d\tBattery level in percentage\n",sel++);
    g_print("%d\tBattery level in percentage with load resistor\n",sel++);
    g_print("%d\tTurn LED on\n",sel++);
    g_print("%d\tTurn LED off\n",sel++);
    g_print("%d\tRead BME280 (if connected)\n",sel++);
    g_print("%d\tRead ADC channel\n",sel++);
    g_print("%d\tRead a digital pin\n",sel++);
    g_print("%d\tSet a pin HIGH\n",sel++);
    g_print("%d\tSet a pin LOW\n",sel++);
    g_print("%d\tPerform simple chat\n",sel++);
    g_print("30\tRequest server version number\n");
}

/**
 * @brief read keyboard input interactive
 *
 * Make sure the selection matches the display_menu() order
 *
 * NOTE
 * The boolean (like op_tempC) are defined in amdtc.c and indicated
 * as external in top of this program.
 */
gboolean read_interactive()
{
    int selection = 0, ret;

    do {
        // in case of sending test data the keyboard in non-blocking
        if (opt_start_data) {
            selection = rd_keyboard();
            if (selection == 0) return TRUE;
        }
        else {
            display_menu();
            g_print("\nProvide your choice : ");
            ret = scanf("%d", &selection);
        }

        ret = 1;
        switch (selection) {
            case 0:
                return FALSE;
                break;
            case 1: // Start test data
                opt_start_data = TRUE;
                set_terminal();
                break;
            case 2: // Stop test data
                rst_terminal();
                opt_start_data = FALSE;
                opt_stop_data = TRUE;
                break;
            case 3: // read tempC
                opt_TempC = TRUE;
                break;
            case 4: // read tempF
                opt_TempF = TRUE;
                break;
            case 5: // read battery
                opt_Batt = TRUE;
                break;
            case 6: // read battery with load resistor
                opt_Batt_load = TRUE;
                break;
            case 7: // Turn led on
                opt_TurnOn = TRUE;
                break;
            case 8: // Turn led off
                opt_TurnOff = TRUE;
                break;
            case 9: // read BME280
                opt_bme280 = TRUE;
                break;
            case 10: // read ADC input
                opt_adc = TRUE;
                break;
            case 11: // read digital pin
                opt_read_pin = TRUE;
                break;
            case 12: // digital pin high
                opt_pin_high = TRUE;
                break;
            case 13: // digital pin low
                opt_pin_low = TRUE;
                break;
            case 14: // simple chat
                opt_chat = TRUE;
                FirstChat = TRUE;
                break;
            case 30: // Server version number
                opt_vers = TRUE;
                break;
            default:
                g_printerr("Invalid selection: 0x%x or %c\n",selection, selection);
                ret = 0;
                break;
        }
    }  while (ret == 0);

    return TRUE;
}

/**
 * set keyboard as none blocking when receiving test data
 */
void set_terminal()
{
    set_conio_terminal_mode();
    ConioSet = TRUE;
}

/**
 * set keyboard as none blocking
 */
void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(rst_terminal);
    new_termios.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &new_termios);
}

/**
 * reset the keyboard to original when stop receiving test data
 */
void rst_terminal()
{
    if (ConioSet) {
        tcsetattr(0, TCSANOW, &orig_termios);
        ConioSet = FALSE;
    }
}

/**
 * read keyboard input none-blocking. Wait for 1 second for input
 */
int rd_keyboard()
{
    struct timeval tv;
    fd_set fds;
    int ret, i = 0 ;
    char c;
    char buf[5];

    // wait for 1 second to check for input
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    // watch stdin for input
    FD_ZERO(&fds);
    FD_SET(0, &fds);

    ret = select(1, &fds, NULL, NULL, &tv);

    if (ret == -1) {
        g_print("error during reading keyboard\n");
        return -1;
    }

    else if (ret) {

        // read keyboard
        if (read(0, &c, sizeof(c)) < 0) return 0;
        return c - '0';
    }
    else
        return 0;
}
