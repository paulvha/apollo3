/*
 * paulvha/ February 2020 / version 1.0.1
 * # update to timer handling
 */
 
#include "BLE_example.h"

extern "C" void set_adv_name( const char* str );
void setAdvName(const char* str){
    set_adv_name( str );
}

//*****************************************************************************
//
// Timer configuration macros.
//
//*****************************************************************************
#define MS_PER_TIMER_TICK           10  // Milliseconds per WSF tick
#define CLK_TICKS_PER_WSF_TICKS     5   // Number of CTIMER counts per WSF tick.

//*****************************************************************************
//
// WSF buffer pools.
//
//*****************************************************************************
#define WSF_BUF_POOLS               4

// Important note: the size of g_pui32BufMem should includes both overhead of internal
// buffer management structure, wsfBufPool_t (up to 16 bytes for each pool), and pool
// description (e.g. g_psPoolDescriptors below).

// Memory for the buffer pool
static uint32_t g_pui32BufMem[(WSF_BUF_POOLS*16
         + 16*8 + 32*4 + 64*6 + 280*8) / sizeof(uint32_t)];

// Default pool descriptor.
static wsfBufPoolDesc_t g_psPoolDescriptors[WSF_BUF_POOLS] =
{
    {  16,  8 },
    {  32,  4 },
    {  64,  6 },
    { 280,  8 }
};


//*****************************************************************************
//
// Tracking variable for the scheduler timer.
//
//*****************************************************************************
uint32_t g_ui32LastTime = 0;


//*****************************************************************************
//
// Initialization for the ExactLE stack.
//
//*****************************************************************************
void exactle_stack_init(void){
    wsfHandlerId_t handlerId;

    //
    // Set up timers for the WSF scheduler.
    //
    scheduler_timer_init();
    WsfTimerInit();

    //
    // Initialize a buffer pool for WSF dynamic memory needs.
    //
    WsfBufInit(sizeof(g_pui32BufMem), (uint8_t*)g_pui32BufMem, WSF_BUF_POOLS, g_psPoolDescriptors);

    //
    // Initialize security.
    //
    SecInit();
    SecAesInit();
    SecCmacInit();
    SecEccInit();

    //
    // Set up callback functions for the various layers of the ExactLE stack.
    //
    handlerId = WsfOsSetNextHandler(HciHandler);
    HciHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(DmHandler);
    DmDevVsInit(0);
    DmAdvInit();
    DmConnInit();
    DmConnSlaveInit();
    DmSecInit();
    DmSecLescInit();
    DmPrivInit();
    DmHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(L2cSlaveHandler);
    L2cSlaveHandlerInit(handlerId);
    L2cInit();
    L2cSlaveInit();

    handlerId = WsfOsSetNextHandler(AttHandler);
    AttHandlerInit(handlerId);
    AttsInit();
    AttsIndInit();
    AttcInit();

    handlerId = WsfOsSetNextHandler(SmpHandler);
    SmpHandlerInit(handlerId);
    SmprInit();
    SmprScInit();
    HciSetMaxRxAclLen(251);

    handlerId = WsfOsSetNextHandler(AppHandler);
    AppHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(TagHandler);
    TagHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(HciDrvHandler);
    HciDrvHandlerInit(handlerId);
}

//*****************************************************************************
//
// Set up a pair of timers to handle the WSF scheduler.
//
//*****************************************************************************
void
scheduler_timer_init(void)
{
    //
    // The timer will run continuously and provide a constant time-base.
    //
    am_hal_ctimer_clear(0, AM_HAL_CTIMER_TIMERB);
    am_hal_ctimer_config_single(0, AM_HAL_CTIMER_TIMERB,
                                 (AM_HAL_CTIMER_LFRC_512HZ |
                                 AM_HAL_CTIMER_FN_CONTINUOUS));

    //
    // Start the continuous timer.
    //
    am_hal_ctimer_start(0, AM_HAL_CTIMER_TIMERB);

}

//*****************************************************************************
//
// Calculate the elapsed time, and update the WSF software timers.
//
//*****************************************************************************
void
update_scheduler_timers(void)
{
    uint32_t ui32CurrentTime, ui32ElapsedTime;

    //
    // Read the continuous timer.
    //
    ui32CurrentTime = am_hal_ctimer_read(0, AM_HAL_CTIMER_TIMERB);

    //
    // Figure out how long it has been since the last time we've read the
    // continuous timer. We should be reading often enough that we'll never
    // have more than one overflow.
    //
    ui32ElapsedTime = (ui32CurrentTime >= g_ui32LastTime ?
                       (ui32CurrentTime - g_ui32LastTime) :
                       (0x10000 + ui32CurrentTime - g_ui32LastTime));

    //
    // Check to see if any WSF ticks need to happen.
    //
    if ( (ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS) > 0 )
    {
        //
        // Update the WSF timers and save the current time as our "last
        // update".
        //
        WsfTimerUpdate(ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS);

        g_ui32LastTime = ui32CurrentTime;
    }
}

//*****************************************************************************
//
// Interrupt handler for the CTIMERs (not really used at the moment)
//
//*****************************************************************************
extern "C" void am_ctimer_isr(void){
    uint32_t ui32Status;

    //
    // Check and clear any active CTIMER interrupts.
    //
    ui32Status = am_hal_ctimer_int_status_get(true);
    am_hal_ctimer_int_clear(ui32Status);

    //
    // Run handlers for the various possible timer events.
    //
    am_hal_ctimer_int_service(ui32Status);
}

//*****************************************************************************
//
// Interrupt handler for BLE
//
//*****************************************************************************
extern "C" void am_ble_isr(void){
    HciDrvIntService();
}

// ****************************************
// 
// Debug print functions
// 
// ****************************************
#ifdef BLE_Debug

#define DEBUG_UART_BUF_LEN 256

extern "C" void debug_float (float f) {
  SERIAL_PORT.print(f);
}
extern "C" void debug_print(const char* f, const char* F, uint16_t L){
  SERIAL_PORT.printf("fm: %s, file: %s, line: %d\n", f, F, L);
}

extern "C" void debug_printf(char* fmt, ...){
    char debug_buffer [DEBUG_UART_BUF_LEN];
    va_list args;
    va_start (args, fmt);
    vsnprintf(debug_buffer, DEBUG_UART_BUF_LEN, (const char*)fmt, args);
    va_end (args);

    SERIAL_PORT.print(debug_buffer);
}
#endif //BLE_Debug  

/****************************************
 * 
 * C-callable read analog signal
 *
 * 1 : return internal battery level
 * 2 : return internal temperature
 * 
 * perform reading multiple times to improve the quality
 ****************************************/
#define RETRY 3
 
extern uint16_t read_adc(uint8_t R) {
  
  int i, j, k, rr, pin;
  uint16_t val = 0;  
  
  if (R == 1) pin = ADC_INTERNAL_VCC_DIV3;  // battery get adc voltage divided by 3
  else if (R == 2) pin = ADC_INTERNAL_TEMP; // get internal temperature value
  else   // extend here if you want to read other ADC values
      return 0;


  // Often analogRead fails (read zero) or provides previous data on Apollo hence 
  // a couple of ways to handle :
  // 1 read first a dummy to flush old reading
  // 2 Retry a couple of times and sent average
  // 3 ignore zero reading for 10 times max.
  
  // read dummy to flush
  analogRead(pin);
  
  j = RETRY;
  k = 0;          // dead loop control

  for(i=0; i < j ; i++) {
    rr = analogRead(pin);
    
    if (rr == 0) {                  // if zero reading
       if (k++ < 10) j++;           // retry once again BUT no more than 10 times
       else return 0;               // dead loop control
    }
    
    else val = val + rr;
    //Serial.print("rr = ");        // enable for debug only
    //Serial.println(rr);
  }
  val = val / (i - k);              // the number of sample might have been extended because of ZERO reading
  return(val & 0x3ff);              // 10 bits is standard, 1024 -> 0x400
}

// ****************************************
// 
// C-callable led functions
// 
// ****************************************
extern void set_led_high( void ){
  digitalWrite(LED_BUILTIN, HIGH);
}

extern void set_led_low( void ){
  digitalWrite(LED_BUILTIN, LOW);
}
