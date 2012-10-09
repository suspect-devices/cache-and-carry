/*------------------------------------------------------------------------Monitor.h
 *
 * The pourpose of the monitor is to interpret and dispatch data The idea is to 
 * try to set up a relatively flexible interpreter for data coming in from the 
 * serial port and also commands coming from the usb port. Using a jump table which
 * is populated by other modules who register the action to be taken. 
 *
 * ------------------------------------------- current vocabulary.
 * Deal with multi line data.
 * SOD[  :]Start of multiline data
 * EOD[  :]End of multiline data
 * SYN[?  ]Say Hello
 * SWV[?  ]Software version
 * 
 * all remaining items are sent to the monitored device
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

#ifndef __MONITOR_H__
#define __MONITOR_H__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <io.h>
//#include <signal.h>
#include <stdint.h>
#include <stdarg.h>
#include <wirish.h>
#include <usart.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#define BOM_VERSION "BOM5K_"__DATE__
//#define BOM_VERSION "BOM5K_GEN_"__DATE__ " "__TIME__

#define KEYWORDS \
"SYN" "ACK" "NAK" "SWV" "LED" "NOP"

enum keywordIndex {
_SYN_,_ACK_,_NAK_,_SWV_,_LED_,_NOP_
};

// use enum to determine the size of the keyword arrays.
#define NKEYWORDS  _NOP_ + 1 
#define MAX_COMMAND_LINE_LENGTH 104

/*-----------------------------------------------------------------DECLARATIONS
 * 
 *----------------------------------------------------------------------------*/

#ifndef uint8_t 
#ifndef uint8
#define uint8 unsigned char
#define uint16 unsigned short int
#endif 
#define uint8_t uint8
#define uint16_t uint16 
//#define uint32_t uint32 
#endif

extern const char *keywords;

typedef uint8_t (actionfunc)(uint8_t source);
typedef uint8_t (*actionptr)(uint8_t source);

extern actionptr actions[];
extern char actionBuffer[MAX_COMMAND_LINE_LENGTH];
//extern const char *theNak;

typedef struct __cmdbuff {
    unsigned char line[MAX_COMMAND_LINE_LENGTH];
    unsigned char * args;
    unsigned char key[4]; //unused at present. 
    unsigned char verb;
    int kwIndex;
    bool gotline;
    uint8_t len;
} cmdBuffer;


extern cmdBuffer deviceComm;
extern cmdBuffer consoleComm;

enum sourceTypes {
    CONSOLE,
    DEVICE 
};
enum inputReturnValues {
    COMMAND_ERROR=-1,
    COMMAND_OK,
    COMMAND_FORWARDED,
    COMMAND_IGNORED
};

int get_free_memory();
int lookupIndex(unsigned char *key); 
char *lookupKey(int ndx, unsigned char *keyBuffer); 
int handleDeviceInput(cmdBuffer *);
int handleConsoleInput(cmdBuffer *);
void toConsole(const char *format, ...);
void toDevice(const char *format, ...);
void registerAction(uint8_t, actionptr);
char *actBuff(const char *format, ...);
#endif
