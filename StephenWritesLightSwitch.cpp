/*-----------------------------------------------------------------datalogger.cpp
 *
 * This is a functional prototype of a datalogger. 
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
// should move this to common.h or something silly

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
#include <SdFat.h>
#include <HardwareSPI.h>
#include <wirish/wirish.h>
#include <libmaple/timer.h>
#include <libmaple/usart.h>
//extern "C" {
#include "Monitor.h"
#include "Utilities.h"
//}

/* Global Variables */
/*-------------------------------------------------------------GLOBAL VARIABLES
 * 
 *----------------------------------------------------------------------------*/

HardwareSPI spi(1);
//Sd2Card card(&spi);
//Sd2Card card;HardwareSPI spi(1);
Sd2Card card(spi, USE_NSS_PIN);

SdVolume volume;
SdFile root;
SdFile file;


SdFile settingsFile;
SdFile sysinfoFile;
char logFileName[16];
char settingsFileName[16]="BOM5K.SET";
char sysinfoFileName[16]="AMAV2.DAT";
bool connectedToNet=false;
bool logOpened=false;
time_t epoch=0;

#define MAX_SSN_LENGTH 16
#define MAX_HWV_LENGTH 8
#define MAX_SWV_LENGTH 24
#define MAX_TIME_LENGTH 24


char deviceSSN[MAX_SSN_LENGTH]="00000000100";
char deviceHWV[MAX_HWV_LENGTH]="0";
char deviceSWV[MAX_SWV_LENGTH]="0.0-000000";
time_t deviceLastContact=0L;
bool deviceIsConnected=false;
int sdCardStatus=0;
const int sensor_pin = 12;


/*---------------------------------------------------------------hardwareSetup()
 * 
 *----------------------------------------------------------------------------*/

void hardwareSetup(void) {
    spi.begin(SPI_281_250KHZ, MSBFIRST, 0); // move to sdcard init?
    
    pinMode(YEL_LED, OUTPUT);
    pinMode(GRN_LED, OUTPUT);
    pinMode(BLU_LED, OUTPUT);
    digitalWrite(YEL_LED, HIGH);
    digitalWrite(BLU_LED, LOW);
    digitalWrite(GRN_LED, HIGH);
    SerialUSB.begin();  
    Serial1.begin(9600);  
    Serial2.begin(9600); 
    // probably should initialize the monitor.
    sdCardStatus=sdCardInit();
    
}
/*-----------------------------------------------------------------------Setup()
 * 
 *----------------------------------------------------------------------------*/

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
    
    registerAction(_DIR_, DIRaction);

    timer.pause();
    
    // Set up period
    timer.setPeriod(1000000); // in microseconds
    
    // Set up an interrupt on channel 1
    timer.setMode(TIMER_CH1,TIMER_OUTPUT_COMPARE);
    timer.setCompare(TIMER_CH1, 1);  // Interrupt 1 count after each update
    timer.attachInterrupt(TIMER_CH1,ledTask);
    
    
    // Refresh the timer's count, prescale, and overflow
    timer.refresh();
    
    // Start the timer counting
    timer.resume();
    
}

/*---------------------------------------------------------------------LedTask()
 * stubb
 *----------------------------------------------------------------------------*/

static void ledTask()
{
    togglePin(BLU_LED);
}

static void serialTasks() {
    char ch;
    setupKeywords();
    while ((consoleComm.gotline==false) && (SerialUSB.available())) {
        if ((((ch=SerialUSB.read())!='\r') && (ch!='\n'))
            &&(consoleComm.len<MAX_COMMAND_LINE_LENGTH)
            ) {
            consoleComm.line[consoleComm.len++] = ch;
        } else {
            if (consoleComm.len) {
                consoleComm.gotline = true;
            }
            consoleComm.line[consoleComm.len]='\0';
        }
        //SerialUSB.write(ch);
    }
    while ((deviceComm.gotline==false) && (Serial1.available())) {
        if ((((ch=Serial1.read())!='\r') && (ch!='\n'))
            &&(deviceComm.len<MAX_COMMAND_LINE_LENGTH)
            ) {
            deviceComm.line[deviceComm.len++] = ch;
        } else {
            if (deviceComm.len) {
                deviceComm.gotline = true;
            }
            deviceComm.line[deviceComm.len]='\0';
        }
        //SerialUSB.write(ch);
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
        serialTasks(); //move to 
        if (consoleComm.gotline) {
            handleConsoleInput(&consoleComm);
        }
        if (deviceComm.gotline) {
            handleDeviceInput(&deviceComm);
        }
    
            
   }
    
    return 0;
    
}