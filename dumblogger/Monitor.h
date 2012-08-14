
/*------------------------------------------------------------------------Monitor.h
 *
 * 
 *
 #define LOGGERKEYWORDS \
 "DIR"         "DEL" "TX2" "TYP" "NSC" "NJN" "NPW" "NST" "TPT" "FMT" "PKY"\
 "PFD" "PTO" "PIP" "PON" "POF" "SSV" "SGT" "TFN" "TFD" "LTM" "DVP" "DVT"\
 "DTM" "DID" "LVB" "LSV" 
 
 * Deal with multi line data.
 * SOD[  :]Start of multiline data
 * EOD[  :]End of multiline data
 *
 * Log file and disk commands
 * DIR[?  ] directory of the disk.
 * TYP[?  ]<fname> type file
 * DEL[ ! ]<fname> delete file
 * DDS[?  ] dates from logs.
 * DMP[ ! ] dump all logs [in order]
 * 
 * DVP[?  ] device is present?
 * DID[?  ] ssn of device?
 * DVT[?  ] timestamp for last device contact.
 * 
 * LOG[ !:] <data line> log data to current log file
 * LSV[? :] logger software version
 * LTS[ ! ] sync time with client
 * LNT[ ! ] sync time from network.
 * LTM[? :] logger rtc time
 *
 * TX2[ ! ]<text sent to radio on tx2>
 * NSC[?  ] network scan
 * NPW[ ! ]<passwd> network password
 * NJN[ ! ]<ssid> join network.
 * NST[?  ] network status.
 *
 * PFD[?!:] pachube feed
 * PKY[?!:] pachube key
 * PIP[?!:] pachube server ip
 *
 * TS[1-5][ :] temp data sent to pachube
 * TPT[ ! ]<data> test data sent to pachube slot #1
 * FAN[  :]<fan setting>
 * CHL[  :]<chiller setting>
 * STC[  :]<state#> <Description> state changes sent to pachube (slots #6 and #7)
 *
 * SSV[ ! ] save settings (read	 bom5k.set)
 * SGT[?  ] get settings  (write bom5k.set)
 * 
 * all remaining items are sent to the monitored device
 * 
 *-----------------------------------------------------------------------------*/

#ifndef __MONITOR_H__
#define __MONITOR_H__ 1
#include <stdio.h>
#include <io.h>
#include <signal.h>
#include <stdint.h>
#include <stdarg.h>
#include <wirish/wirish.h>
#include <libmaple/usart.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#define BOM_VERSION "BOM5K_"__DATE__
//#define BOM_VERSION "BOM5K_"__DATE__ " "__TIME__

#define KEYWORDS \
"SYN" "ACK" "NAK" "SWV" "HWV" "MEM" "SSN" "HLP" "FTL" "ALT" "WRN" "INF" "DBG"\
"LOG" "STC" "DVL" "LVL" "DLG" "RST" "SHD" "ALM" "CPT" "CND" "BTN" "WBP" "ERF"\
"NOW" "TIM" "THR" "THM" "DFR" "TS1" "TS2" "TS3" "TS4" "TS5" "AMT" "CHT" "AIT"\
"FAN" "CHL" "???" "PWR" "PCT" "TSC" "CPR" "CPP" "CPI" "CPD" "FPP" "FPI" "FPD"\
"RED" "GRN" "BLU" "TSS" "NOP"

enum keywordIndex {
_SYN_,_ACK_,_NAK_,_SWV_,_HWV_,_MEM_,_SSN_,_HLP_,_FTL_,_ALT_,_WRN_,_INF_,_DBG_,
_LOG_,_STC_,_DVL_,_LVL_,_DLG_,_RST_,_SHD_,_ALM_,_CPT_,_CND_,_BTN_,_WBP_,_ERF_,
_NOW_,_TIM_,_THR_,_THM_,_DFR_,_TS1_,_TS2_,_TS3_,_TS4_,_TS5_,_AMT_,_CHT_,_AIT_,
_FAN_,_CHL_,_TMP_,_PWR_,_PCT_,_TSC_,_CPR_,_CPP_,_CPI_,_CPD_,_FPP_,_FPI_,_FPD_,
_RED_,_GRN_,_BLU_,_TSS_,_NOP_
};

// use enum to determine the size of the keyword arrays.
#define NKEYWORDS  _NOP_ + 1 
//NKEYWORDS;
#define LOGGERKEYWORDS \
"DIR"\
"DEL" "TX2" "TYP" "NSC" "NJN" "NPW" "NST" "TPT" "FMT" "PKY"\
"PFD" "PTO" "PIP" "PON" "POF" "SSV" "SGT" "TFN" "TFD" "LTM" "DVP" "DVT"\
"DTM" "DID" "LVB" "LSV" 


enum loggerKeywordIndex {
_DIR_=_NOP_+1,
_DEL_,_TX2_,_TYP_,_NSC_,_NJN_,_NPW_,_NST_,_TPT_,_FMT_,_PKY_,
_PFD_,_PTO_,_PIP_,_PON_,_POF_,_SSV_,_SGT_,_TFN_,_TFD_,_LTM_,_DVP_,_DVT_,
_DTM_,_DID_,_LVB_,_LSV_
};

//int patchubeIndex[]={_TS1_,_TS2_,_TS3_,_TS4_,_TS5_,_FAN_,_CHL_,_STC_};

#undef NKEYWORDS
#define NKEYWORDS  _LSV_ + 1 
#define MAX_COMMAND_LINE_LENGTH 104

/*-----------------------------------------------------------------DECLARATIONS
 * 
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <wirish/wirish.h>

//#include "DataStream.h"
/* Scheduler includes. */
#include "MapleFreeRTOS.h"
//#include <libmaple/timer.h>
//#include <libmaple/usart.h>

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
//extern  char *keywords;

typedef uint8_t (actionfunc)(uint8_t source);
typedef uint8_t (*actionptr)(uint8_t source);

extern actionptr actions[];
extern char actionBuffer[MAX_COMMAND_LINE_LENGTH];
extern const char *theNak;

typedef struct __cmdbuff {
    unsigned char line[MAX_COMMAND_LINE_LENGTH];
    unsigned char * args;
    unsigned char key[4]; //unused at present. 
    unsigned char verb;
    int kwIndex;
    bool gotline;
    uint8_t len;
    xSemaphoreHandle xGotLineSemaphore;
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
void toConsole(char *);
void toDevice(char *);
void registerAction(uint8_t, actionptr);
char *actBuff(const char *format, ...);
#endif
