/*--------------------------------------------------------------------Monitor.c 
 *  
 *  Description and docs... are in Monitor.h
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

#include "Monitor.h"

const char *keywords = KEYWORDS LOGGERKEYWORDS;

char actionBuffer[MAX_COMMAND_LINE_LENGTH]; //return buffer for console actions allocate this once use it often. 
const char *theNak="NAK:";

cmdBuffer deviceComm;
cmdBuffer consoleComm;

char keyBuff[4]; //fix this.

void registerAction(uint8_t ndx, actionptr action){
    if (ndx >=0 && ndx<NKEYWORDS) {
        actions[ndx]=action;
    }
}

int lookupIndex(unsigned char *key) {
    int ndx=0;
    int ptr=0;
    while (ndx<NKEYWORDS) {
        if ((keywords[ptr]==toupper(key[0])) 
            && (keywords[ptr+1]==toupper(key[1])) 
            && (keywords[ptr+2]==toupper(key[2]))
            ) { return ndx;
        } else {
            ndx++;
            ptr+=3;
        }
    }
    return -1;
}

char * lookupKey(int ndx, char *keyBuff) {
    int ptr=0;
    keyBuff[0]=keyBuff[1]=keyBuff[2]='?';
    if (ndx >=0 && ndx<NKEYWORDS) {
        ptr=ndx*3;
        keyBuff[0]=keywords[ptr]; 
        keyBuff[1]=keywords[ptr+1]; 
        keyBuff[2]=keywords[ptr+2]; 
        keyBuff[3]='\0';
    }
    
    return keyBuff;
}

void toDevice(char *buff){ 
    usart_tx(USART1, (uint8_t *)buff, strlen(buff));
    usart_tx(USART1, (uint8_t *)"\r\n", 2);
    ;
}
extern "C" {int sdoPrintf(char *out, const char *format, ...);}

void toConsole(const char *format, ...){ 
    va_list args;    
    va_start( args, format );
    vsprintf(actionBuffer, format, args );
    if(SerialUSB.isConnected() && (SerialUSB.getDTR() || SerialUSB.getRTS())) {        
        SerialUSB.write(actionBuffer);
        SerialUSB.write("\r\n");
    }
    va_end(args);

}

uint8_t NOPaction(uint8_t source) {  
    if (source==CONSOLE) {
        toConsole("DBG: Forwarding (%c%c%c)!", 
                  consoleComm.line[0], consoleComm.line[1], consoleComm.line[2]);
        toDevice((char *)consoleComm.line);

        return COMMAND_FORWARDED;
    }
    else if (source == DEVICE) {
        toConsole((char *)deviceComm.line);
    } else {
        return COMMAND_IGNORED;
    }
}
uint8_t ACKaction (uint8_t source) {
    if (source==CONSOLE) {
         toConsole("ACK:");
        return COMMAND_OK;
    } else {
        toDevice("ACK:");
        return COMMAND_OK;
    }
}   

uint8_t NAKaction (uint8_t source) {
    if (source==CONSOLE) {
        toConsole("NAK:");
        return COMMAND_OK;
    } else {
        toDevice("NAK:");
        return COMMAND_OK;
    }
}    

uint8_t LSVaction (uint8_t source) {
    if (source==CONSOLE) {
        toConsole("LSV:%s",BOM_VERSION);
        return COMMAND_OK;
    } else {
        toDevice("NAK:");
        return COMMAND_IGNORED;
    }
}    

extern char _lm_heap_end;
extern char _lm_heap_start;
extern caddr_t _sbrk(int incr); //this doesnt resolve. 

int getFreeMemory() // none of this works. 
{   static int freeMemory;
    //if (freeMemory-(int)((caddr_t)&_lm_heap_end) - (int)((caddr_t)&_lm_heap_start)) { SerialUSB.write("!");}
    //freeMemory=(int)((caddr_t)&_lm_heap_end) - (int)((caddr_t)&_lm_heap_start);
    freeMemory=(int)((caddr_t)&_lm_heap_end) - (int)(&freeMemory);
    return freeMemory;
}


uint8_t MEMaction (uint8_t source) {
    if (consoleComm.verb=='?') {
        int memory;
        memory=getFreeMemory();
        toConsole(actionBuffer,"MEM:%d",memory);;
        return COMMAND_OK;
    } else {
        toConsole("NAK:");
        return COMMAND_ERROR;
    }

}


actionptr actions[NKEYWORDS] = {
    &ACKaction,    //SYN [!  ] sync (hello)
    &ACKaction,    //ACK [ : ] acknowlege (yes)
    &NAKaction,    //NAK [ : ] negative ack (no)
    &NOPaction,    //SWV [ :?] Software Version
    &NOPaction,    //HWV [ :?] Hardware Version
    &MEMaction,    //MEM [ :?] Avaliable Memory
    &NOPaction,    //SSN [ :?] Serial Number
    &NOPaction,    //HLP [!: ] help
    &NOPaction,    //FTL [!: ] Fatal Error
    &NOPaction,    //ALT [!: ] Alert
    &NOPaction,    //WRN [!: ] Warning
    &NOPaction,    //INF [!: ] Info
    &NOPaction,    //DBG [!: ] Debugging Info
    &NOPaction,    //LOG [!: ] Log
    &NOPaction,    //STC [!: ] State Change
    &NOPaction,    //DVL [!:?] Display Level
    &NOPaction,    //LVL [!:?] Log Level
    &NOPaction,    //DLG [!:?] Dumb Logger
    &NOPaction,    //RST [!  ] Reboot the system
    &NOPaction,    //SHD [!: ] Shutdown 
    &NOPaction,    //ALM [!  ] Alarm Sound
    &NOPaction,    //CPT [!  ] UI Print
    &NOPaction,    //CND [!  ] UI Indicate
    &NOPaction,    //BTN [ :?] Button Press
    &NOPaction,    //WBP [!: ] Wait for Button
    &NOPaction,    //ERF [!:?] UI Error Flag
    &NOPaction,    //NOW [!:?] Time as a long int
    &NOPaction,    //TIM [ :?] Time as a string
    &NOPaction,    //THR [ :?] Time as a string
    &NOPaction,    //THM [ :?] Time as a string
    &NOPaction,    //DFR [!: ] Defrost
    &NOPaction,    //TS1 [ :?] Temp Sensor 1
    &NOPaction,    //TS2 [ :?] Temp Sensor 2
    &NOPaction,    //TS3 [ :?] Temp Sensor 3
    &NOPaction,    //TS4 [ :?] Temp Sensor 4
    &NOPaction,    //TS5 [ :?] Temp Sensor 5
    &NOPaction,    //AMT [ :?] Temp Sensor 6
    &NOPaction,    //CHT [ :?] Temp Sensor 7
    &NOPaction,    //AIT [ :?] Temp Sensor 8
    &NOPaction,    //FAN [!:?] Fan Output
    &NOPaction,    //CHL [!:?] Chiller Out 
    &NOPaction,    //TMP [ :?] Temperature 
    &NOPaction,    //PWR [!:?] Chiller Power
    &NOPaction,    //PCT [ :?] Percent 
    &NOPaction,    //TSC [   ] Rescan Temp Sensors
    &NOPaction,    //CPR [   ] Chiller Pid Reset 
    &NOPaction,    //CPP [   ] Chiller Pid P
    &NOPaction,    //CPI [   ] Chiller Pid I
    &NOPaction,    //CPD [   ] Chiller Pid D
    &NOPaction,    //FPP [   ] Fan Pid P
    &NOPaction,    //FPI [   ] Fan Pid I 
    &NOPaction,    //FPD [   ] Fan Pid D
    &NOPaction,    //RED [   ] 
    &NOPaction,    //GRN [   ] 
    &NOPaction,    //BLU [   ] 
    &NOPaction,    //TSS [   ] 
    &NOPaction,    //NOP [   ] 
    &NOPaction,    //DIR [   ] 
    &NOPaction,    //DEL [   ] 
    &NOPaction,    //TX2 [   ] 
    &NOPaction,    //TYP [   ] 
    &NOPaction,    //NSC [   ] 
    &NOPaction,    //NJN [   ] 
    &NOPaction,    //NPW [   ] 
    &NOPaction,    //NST [   ] 
    &NOPaction,    //TPT [   ] 
    &NOPaction,    //FMT [   ] 
    &NOPaction,    //PKY [   ] 
    &NOPaction,    //PFD [   ] 
    &NOPaction,    //PTO [   ] 
    &NOPaction,    //PIP [   ] 
    &NOPaction,    //PON [   ] 
    &NOPaction,    //POF [   ] 
    &NOPaction,    //SSV [   ] 
    &NOPaction,    //SGT [   ] 
    &NOPaction,    //TFN [   ] 
    &NOPaction,    //TFD [   ] 
    &NOPaction,    //LTM [   ] 
    &NOPaction,    //DVP [   ] 
    &NOPaction,    //DVT [   ] 
    &NOPaction,    //DTM [   ] 
    &NOPaction,    //DID [   ] 
    &NOPaction,    //LVB [   ] 
    &LSVaction,    //LSV [   ] logger software version. 
};
    
struct tm theTime;

void parseLine(cmdBuffer *buff) {
    // should check for more than 4 characters
    buff->args=(buff->line)+4; 
    buff->verb=buff->line[3];
    //buff->line[3]='\0';
    buff->kwIndex=lookupIndex(buff->line);
}
    
int handleDeviceInput(cmdBuffer * cmd) {
    int retval;
    parseLine(cmd);
    if ((cmd->kwIndex)>0) {
        retval=actions[cmd->kwIndex](DEVICE);
    }
    cmd->gotline=false;
    cmd->len=0;
   return retval;
}

int handleConsoleInput(cmdBuffer * cmd) {
    int retval;
    parseLine(cmd);
 
    if ((cmd->kwIndex) >=0 ) {
        retval=actions[cmd->kwIndex](CONSOLE);
    }
    cmd->gotline=false;
    cmd->len=0;
    return retval;
}
