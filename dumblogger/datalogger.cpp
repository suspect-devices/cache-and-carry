/*-----------------------------------------------------------------datalogger.cpp
 *
 * This is a functional prototype of a datalogger. 
 *
 *
 *
 *-----------------------------------------------------------------------------*/

#ifndef uint8_t 
#define uint8_t uint8
#define uint16_t uint16 
//#define uint32_t uint32 
#endif

#define ASCII_DELETE '\177'
#define ASCII_BACKSPACE '\010'

/*---------------------------------------------------------------------TUNABLES
 * 
 *----------------------------------------------------------------------------*/

#define BLU_LED 19
#define YEL_LED 18
#define GRN_LED 14
#define LED_ON  LOW
#define LED_OFF HIGH
/**
 * \file setup.cpp 
 */
#include <stdio.h>
#include <io.h>
#include <signal.h>
#include <stdint.h>
#include <wirish/wirish.h>
#include "MapleFreeRTOS.h"
#include <libmaple/timer.h>
#include <libmaple/usart.h>
//extern "C" {
#include "Monitor.h"
//}
/* Function Prototypes */
static void vSerialTask(void* pvParameters);
static void vMonitorTask(void* pvParameters);
static void vLedTask(void* pvParameters);

/* Global Variables */


const int sensor_pin = 12;
void hardwareSetup(void) {
    pinMode(YEL_LED, OUTPUT);
    pinMode(GRN_LED, OUTPUT);
    pinMode(BLU_LED, OUTPUT);
    digitalWrite(YEL_LED, HIGH);
    digitalWrite(BLU_LED, LOW);
    digitalWrite(GRN_LED, HIGH);
    SerialUSB.begin();    
}
/**
 * The main function.
 */
void setup( void )
{
    hardwareSetup();
    // Setup the LED to steady on
    pinMode(BOARD_LED_PIN, OUTPUT);
    digitalWrite(BOARD_LED_PIN, HIGH);
    
    // Setup the button as input
    pinMode(BOARD_BUTTON_PIN, INPUT);
    digitalWrite(BOARD_BUTTON_PIN, HIGH);
    
    // Setup the sensor pin as an analog input
    pinMode(sensor_pin,INPUT_ANALOG);
    
    
    /* Add tasks to the scheduler */
    xTaskCreate(vSerialTask, (const signed char*)"Serial", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate(vMonitorTask, (const signed char*)"Monitor", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate(vLedTask, (const signed char*)"Led", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    
    /* Start the scheduler. */
    vTaskStartScheduler();
}

void loop( void )
{
}

static void vLedTask(void* pvParameters)
{
    while (true){
    togglePin(BLU_LED);
    vTaskDelay(200);
    }
}
// move these to monitor.....

/*-----------------------------------------------------------------vSerialTask()
 *
 * Handle (usb/serial) from console and monitored device. 
 * The idea here is to handle the incoming characters as quickly as possible. 
 * All data coming from the console or the datalogger
 * is delimited by <CR><LF>. Finding an end of line "gives" a semaphore.
 * 
 * eventually this should be moved down to the 
 */
static void vSerialTask(void* pvParameters)
{
    portTickType xLastWakeTime = xTaskGetTickCount();
    const portTickType xWakePeriod = 50;
    
    usart_init(USART1);
    usart_set_baud_rate(USART1, USART_USE_PCLK, 9600);
    usart_enable(USART1);
    deviceComm.len=0;
    deviceComm.gotline=false;
    //vSemaphoreCreateBinary( deviceComm.xGotLineSemaphore );
    //if( consoleComm.xGotLineSemaphore == NULL )
    //{ // FREAK OUT!!!
    //}
    consoleComm.len=0;
    consoleComm.gotline=false;
    //vSemaphoreCreateBinary( consoleComm.xGotLineSemaphore );
    //if( consoleComm.xGotLineSemaphore == NULL )
    //{ // FREAK THE HELL OUT!!!
    //}
    
    
    //    usart_init(USART2);
    //    usart_set_baud_rate(USART2, USART_USE_PCLK, 9600);
    //    usart_enable(USART2);
    
    while (true)
    {   
        togglePin(YEL_LED);
        uint32_t rc=0;
        unsigned char ch;
#if 0        
        if (!deviceComm.gotline) {
            //rc=usart_data_available(USART1);
            while (usart_data_available(USART1) 
                   && deviceComm.len<(MAX_COMMAND_LINE_LENGTH-1)) {
                ch=usart_getc(USART1);
                
                if (ch=='\r') {  
                    deviceComm.gotline=true;
                    deviceComm.line[deviceComm.len]='\0';
                    //xSemaphoreGive(deviceComm.xGotLineSemaphore);
                    break;
                } else if (ch!='\n') {
                    deviceComm.line[deviceComm.len++]=ch;
                }
            }
        }
#endif       
        if (!consoleComm.gotline) {
            while(SerialUSB.isConnected() && SerialUSB.available()
                  && consoleComm.len<(MAX_COMMAND_LINE_LENGTH-1)) {
                ch=SerialUSB.read();
                SerialUSB.write(ch);
                if (ch=='\r') {  
                    consoleComm.gotline=true;
                    consoleComm.line[consoleComm.len]='\0';
                    //xSemaphoreGive(consoleComm.xGotLineSemaphore);
                    break;
                } else if ((ch==ASCII_DELETE) || (ch==ASCII_BACKSPACE)) {
                    if (consoleComm.len) consoleComm.len--;
                } else if (ch!='\n') {
                    consoleComm.line[consoleComm.len++]=ch;                    
                }
            }
        }

        vTaskDelayUntil(&xLastWakeTime, xWakePeriod);
        
    }
}

#define IN_NO_TIME_AT_ALL (( portTickType ) (5/portTICK_RATE_MS))
#define UNTIL_ITS_ANNOYING (( portTickType ) (550/portTICK_RATE_MS))
static void vMonitorTask(void* pvParameters)
{
    portTickType xLastWakeTime = xTaskGetTickCount();
    
    const portTickType xWakePeriod = 100;
    int ret;
    /* Infinite loop */
    while(true)
    {   //if(deviceComm.gotline){
//        if (xSemaphoreTake(deviceComm.xGotLineSemaphore,IN_NO_TIME_AT_ALL)){
//            handleDeviceInput(&deviceComm);
//            xSemaphoreGive(deviceComm.xGotLineSemaphore);
//        }
        if (consoleComm.gotline){
//        if (xSemaphoreTake(consoleComm.xGotLineSemaphore,IN_NO_TIME_AT_ALL)){
            ret=handleConsoleInput(&consoleComm);
//            xSemaphoreGive(consoleComm.xGotLineSemaphore);
          //  if(ret==COMMAND_FORWARDED){
//                if (xSemaphoreTake(deviceComm.xGotLineSemaphore,UNTIL_ITS_ANNOYING))  {
          //      while (!deviceComm.gotline) {
                    
           //     }
           //         handleDeviceInput(&deviceComm);
//                    xSemaphoreGive(deviceComm.xGotLineSemaphore);
                //}
            //}
         }
        //
        
        
        togglePin(GRN_LED);
        //SerialUSB.println("?");
        toConsole("?");
 //       vTaskDelay(100);
       vTaskDelayUntil(&xLastWakeTime, xWakePeriod);
    }
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}

int main(void) {
    setup();
    // should never get past setup.
    while (true) {
        loop();
    }
    
    return 0;
    
}