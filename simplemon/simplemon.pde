/*-----------------------------------------------------------------simeplemon.pde
 * This is a sample piece of code to discuss code
 *
 * the logger takes info from a monitored device (device) over serial and commands 
 * from a program or a user through a usb console. These commands are formatted
 * in a mannor described in Monitor.cpp and Monitor.h
 *
 *   Copyright (c) 2012, Donald Delmar Davis, Suspect Devices
 *   All rights reserved.
 *    
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Suspect Devices nor the
 *   names of its contributors may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *    
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 *   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#define LED_ON  LOW
#define LED_OFF HIGH

#include "Monitor.h"

/* Global Variables */
/*-------------------------------------------------------------GLOBAL VARIABLES
 * 
 *----------------------------------------------------------------------------*/

time_t epoch=0;

#define MAX_SSN_LENGTH 16
#define MAX_HWV_LENGTH 8
#define MAX_SWV_LENGTH 24
#define MAX_TIME_LENGTH 24

//stuff for the FUTURE!!!!
char deviceSSN[MAX_SSN_LENGTH]="00000000000";
char deviceHWV[MAX_HWV_LENGTH]="0";
char deviceSWV[MAX_SWV_LENGTH]="0.0-000000";
time_t deviceLastContact=0L;
bool deviceIsConnected=false;


/*-----------------------------------------------------------------------Setup()
 * 
 *----------------------------------------------------------------------------*/

void setup( void )
{
    SerialUSB.begin();  
   // Setup the LED to steady on
    pinMode(BOARD_LED_PIN, OUTPUT);
    digitalWrite(BOARD_LED_PIN, HIGH);
    
    // Setup the button as input
    pinMode(BOARD_BUTTON_PIN, INPUT);
    digitalWrite(BOARD_BUTTON_PIN, HIGH);
    usart_init(USART1);
    usart_set_baud_rate(USART1, USART_USE_PCLK, 9600);
    usart_enable(USART1);

    deviceComm.len=0;
    deviceComm.gotline=false;
    consoleComm.len=0;
    consoleComm.gotline=false;
    
    
}

void loop( void )
{
    int ret;
    if (deviceComm.gotline){
        handleDeviceInput(&deviceComm);
    }
    if (consoleComm.gotline){
        ret=handleConsoleInput(&consoleComm);
        if(ret==COMMAND_FORWARDED){ // if the device is not connected we should not wait 
           //  if ((xSemaphoreTake(deviceComm.xGotLineSemaphore,DEVICE_RESPONSE_TIMEOUT) != pdTRUE)
           //      && deviceComm.gotline)  {
           //     handleDeviceInput(&deviceComm);
           // } else {
           //     toConsole("NAK:TIMEOUT");
           // }
            
        }
        togglePin(GRN_LED);
    }
}
/*--------------------------------------------------------------------vLedTask()
 * stubb
 *----------------------------------------------------------------------------*/

void dasBlinkin(void* pvParameters)
{
 }
// move these to monitor.....

/*-----------------------------------------------------------------vSerialTask()
 *
 * Handle (usb/serial) from console and monitored device. 
 * The idea here is to handle the incoming characters as quickly as possible. 
 * All data coming from the console or the datalogger
 * is delimited by <CR><LF>. Finding an end of line "gives" a semaphore.
 * 
 * Eventually this should be moved down to the usart interupt handlers imho
 *
 * At 9600 baud this needs to be run at least every (uart buffer size * ms) 
 * to insure that the incoming data doesnt overrun the buffers.
 *
 */
void handleSerial()
{
    togglePin(YEL_LED);
    uint32_t rc=0;
    unsigned char ch;
    
    if (!deviceComm.gotline) {
        while (usart_data_available(USART1) 
               && deviceComm.len<(MAX_COMMAND_LINE_LENGTH-1)) {
            ch=usart_getc(USART1);
            
            if (ch=='\r') {  
                deviceComm.gotline=true;
                deviceComm.line[deviceComm.len]='\0';
                break;
            } else if (ch!='\n') {
                deviceComm.line[deviceComm.len++]=ch;
            }
        }
    }
    
    if (!consoleComm.gotline) {
        while(SerialUSB.isConnected() && SerialUSB.available()
              && consoleComm.len<(MAX_COMMAND_LINE_LENGTH-1)) {
            ch=SerialUSB.read();
            SerialUSB.write(ch);
            if (ch=='\r') {  
                consoleComm.gotline=true;
                consoleComm.line[consoleComm.len]='\0';
                break;
            } else if ((ch==ASCII_DELETE) || (ch==ASCII_BACKSPACE)) {
                if (consoleComm.len) consoleComm.len--;
            } else if (ch!='\n') {
                consoleComm.line[consoleComm.len++]=ch;                    
            }
        }
    }
}
/*-------------------------------------------------------------------vMonitorTask()
 *
 * The monitor task waits for end of lines from the serial handler and dispatches it.
 * 
 *
 *
 */

static void vMonitorTask(void* pvParameters)
{
// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}

int main(void) {
    setup();
 
    while (true) {
        loop();
    }
    
    return 0;
    
}
