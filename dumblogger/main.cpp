/*------------------------------------------------------------datalogger/main.cpp
 *
 * This is a functional prototype of a datalogger. 
 * 
 *
 * 
 * Loose framework (thinking out loud). 
 *
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
 * DVT[?  ] timestamp for last device contact.
 * 
 * LOG[ !:]<data line> log data to current log file
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
 * TS[1-5][ :] temp data sent to pachube
 * TPT[ ! ]<data> test data sent to pachube slot #1
 * FAN[  :]<fan setting>
 * CHL[  :]<chiller setting>
 * STC[  :]<state#> <Description> state changes sent to pachube (slots #6 and #7)
 *
 * SSV[ ! ] save settings (read	 bom5k.set)
 * SGT[?  ] get settings  (write bom5k.set)
 * 
 * remaining items are sent to
 * 
 *-----------------------------------------------------------------------------*/
#include "wirish.h"
#include "iwdg.h"
#include <ctype.h>
#include <SdFat.h>
#include <SdFat.h>
#include <time.h>
//#include <RTClock.h>
#include <HardwareSPI.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef uint8_t 
#define uint8_t uint8
#define uint16_t uint16 
#define uint32_t uint32 
#endif

/*---------------------------------------------------------------------TUNABLES
 * 
 *----------------------------------------------------------------------------*/

#define GRN_LED 19
#define ORN_LED 18
#define RED_LED 14
#define LED_ON  LOW
#define LED_OFF HIGH
#define MAX_RETURN_VALUE 104

#define WATCHDOG_TIMEOUT 16000
#define  Watchdog_Reset iwdg_feed


#define BOM_VERSION "BOM5K_"__DATE__

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
    "DIR"         "DEL" "TX2" "TYP" "NSC" "NJN" "NPW" "NST" "TPT" "FMT" "PKY"\
      "PFD" "PTO" "PIP" "PON" "POF" "SSV" "SGT" "TFN" "TFD" "LTM" "DVP" "DVT"\
      "LSV" 
    

enum loggerKeywordIndex {
    _DIR_=_NOP_+1,_DEL_,_TX2_,_TYP_,_NSC_,_NJN_,_NPW_,_NST_,_TPT_,_FMT_,_PKY_,
      _PFD_,_PTO_,_PIP_,_PON_,_POF_,_SSV_,_SGT_,_TFN_,_TFD_,_LTM_,_DVP_,_DVT_,
      _LSV_
};

//int patchubeIndex[]={_TS1_,_TS2_,_TS3_,_TS4_,_TS5_,_FAN_,_CHL_,_STC_};

#undef NKEYWORDS
#define NKEYWORDS  _LSV_ + 1 
/*-----------------------------------------------------------------DECLARATIONS
 * 
 *----------------------------------------------------------------------------*/

int toPachube(int, char *);
void readSettings(void);
void writeSettings(void);
int networkStatus();
const char keywords[((NKEYWORDS)*3)+1] = KEYWORDS LOGGERKEYWORDS;

int lookupIndex(char *);
void logData(char *);
void stripcrlf(char *);

/*-------------------------------------------------------------GLOBAL VARIABLES
 * 
 *----------------------------------------------------------------------------*/

HardwareSPI spi(1);
Sd2Card card;
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
bool hasWifiModule=true;

#define MAX_PATCHUBE_KEY_LENGHT 80
#define MAX_PATCHUBE_FEED_LENGHT 10
#define MAX_NETWORK_PASSWORD_LENGTH 40
#define MAX_NETWORK_SSID_LENGTH 64
#define MAX_IP_LENGTH 16
#define MAX_SSN_LENGTH 16
#define MAX_HWV_LENGTH 8
#define MAX_SWV_LENGTH 24
#define MAX_TIME_LENGTH 24


char networkPassword[MAX_NETWORK_PASSWORD_LENGTH];
char networkSSID[MAX_NETWORK_SSID_LENGTH];
char pachubeKey[MAX_PATCHUBE_KEY_LENGHT]="J_4LnoRNleWbZDm9FYp3XevLWXuSzlDiJwCsQnc1TGFQOUZLMHM9";
char pachubeFeed[MAX_PATCHUBE_FEED_LENGHT]="50019";
char pachubeIP[MAX_IP_LENGTH]="216.52.233.122";
char deviceSSN[MAX_SSN_LENGTH]="00000000000";
char deviceHWV[MAX_HWV_LENGTH]="0";
char deviceSWV[MAX_SWV_LENGTH]="0.0-000000";
time_t deviceLastContact=0L;
bool deviceIsConnected=false;

/*------------------------------------------------------------------DataStream::
 * class to handle data streams. 
 *----------------------------------------------------------------------------*/

class DataStream {
private:
    Print *_out;
    bool _isUSB;
    char _buf[MAX_RETURN_VALUE];
    char _key[4];
    uint8_t _keyVal;
    char _action;
    uint8_t _ndx;
    //bool _gotLine;
public:
    DataStream(Print *, int); //initializer
    bool available();
    uint8_t read();
    void printf(const char *format, ...);
    bool gotLine(unsigned int timeout);
    char * line() {return _buf; };
    char action() {return _action; };
    char * key(){return _key; };
    char * arguments() {return _buf+4 ; };
    uint8_t keyValue() {return _keyVal; };
    void puts(char *s); //{ _out->print(s); };
    void putChar(char ch); //{ _out->print(ch); };
	void discard() { while(available())read();_ndx=0;}
	void flush() {    if (_isUSB) {/*SerialUSB.flush();*/;} else {((HardwareSerial *)_out)->flush();}}
};

DataStream::DataStream(Print * sout, int baudRate=0) {
    _out=sout;
    _isUSB=(baudRate==0);
    _ndx=0;
    //_gotLine=false;
    _buf[0]=_key[0]='\0';
    if (_isUSB) {
        SerialUSB.begin();
    } else {
        ((HardwareSerial *)_out)->begin(baudRate);
    }
    
}

char outputBuffer[MAX_RETURN_VALUE]; //allocate once use often.

void DataStream::printf(const char *format, ...){ 
    va_list blarg;
    va_start(blarg, format);
    vsnprintf(outputBuffer, MAX_RETURN_VALUE, format, blarg);
    va_end(blarg);
//    
    //if(SerialUSB.isConnected() && (SerialUSB.getDTR() || SerialUSB.getRTS())) {
    puts(outputBuffer);
}

void DataStream::puts(char *s) { 
    if (_isUSB && (!SerialUSB.isConnected() || (!SerialUSB.getDTR() && !SerialUSB.getRTS()))) 
        return;
    _out->write(s); 
};

void DataStream::putChar(char ch) {
    if (_isUSB && (!SerialUSB.isConnected() || (!SerialUSB.getDTR() && !SerialUSB.getRTS()))) 
        return; 
    _out->write(&ch,1); };


bool DataStream::available() {
    if (_isUSB) {
        return ((USBSerial *)_out)->available();
    } else {
        return ((HardwareSerial *)_out)->available();
    }
}

uint8_t DataStream::read() {
    if (_isUSB) {
        return (uint8_t) ((USBSerial *)_out)->read();
    } else {
        return (uint8_t) ((HardwareSerial *)_out)->read();
    }
}

bool DataStream::gotLine(unsigned int timeout=5000) {
    char ch=0;
    bool retval=false;
	unsigned long int mark=millis();
    if(available()){
        while( (millis()-mark < timeout) && available() && _ndx < 99 && ((ch=read()) != '\n') && ch !='\r' ) {
            _buf[_ndx++] = ch; 
        }        
        if (_ndx && ((ch=='\n')||(ch=='\r'))) { // should probably check if line is at least 4 chars.
            _buf[_ndx++]='\r';
            _buf[_ndx++]='\n';
            _buf[_ndx]='\0';
            _ndx=0;
            //_gotLine=
            retval=true;
            _key[0]=_buf[0];
            _key[1]=_buf[1];
            _key[2]=_buf[2];
            _key[3]='\0';
            _action=_buf[3];
            _keyVal=lookupIndex(_key);
        }
    }
    return retval;
}    

DataStream device(&Serial1,9600);
DataStream console(&SerialUSB);
DataStream radio(&Serial2,9600);

/*----------------------------------------------------------------MISC ROUTINES
 *
 *----------------------------------------------------------------------------*/

void stripcrlf(char *mystring) {
    char * finger;
    if ((finger=strchr(mystring,'\r')) != NULL) {
        finger[0]='\0';
    }
    if ((finger=strchr(mystring,'\n')) != NULL) {
        finger[0]='\0';
    }
}

/*-----------------------------------------------------------handleDeviceInput()
 * handle data coming from device. 
 * temperature sensor, fan, chiller, and state data is sent to pachube.
 * log data is sent to the sd card. 
 *----------------------------------------------------------------------------*/
void logData(char *data) {
    int n;
    togglePin(GRN_LED);
    n=file.write(data,strlen(data));
    console.printf("DBG: Logging (%d)'%s'",n,data);
    file.sync();
    togglePin(GRN_LED);
}

void clearStowedValues();

volatile long unsigned int throttleMark;

char stowValues[7][16];
long unsigned int stowTimes[7];
#define TROTTLE_MILLIS 2000
void handleDeviceInput() {
    bool throttle = false;
    bool dataSent = false;
    if ((millis()-throttleMark<TROTTLE_MILLIS)) { 
        throttle=true;
    }
    switch (device.keyValue()) {
        case _LOG_:
            logData(device.arguments());                
            break;
        case _SWV_:
            strncpy(deviceSWV, device.arguments(), MAX_SWV_LENGTH-1);
            break;
        case _HWV_:
            strncpy(deviceHWV, device.arguments(), MAX_HWV_LENGTH-1);
            break;
        case _SSN_:
            strncpy(deviceSSN, device.arguments(), MAX_SSN_LENGTH-1);
            break;
        case _TS1_: 
            if (!throttle) {
                dataSent=true;
                toPachube(0, device.arguments());
            } else {
                strncpy(stowValues[0],device.arguments(),16);
                if (!stowTimes[0]) stowTimes[0]=millis();
            }
            break;
        case _TS2_: 
            if (!throttle) {
                dataSent=true;
                toPachube(1, device.arguments());
            } else {
                strncpy(stowValues[1],device.arguments(),16);
                if (!stowTimes[1]) stowTimes[1]=millis();
            }
            break;
        case _TS3_: 
            if (!throttle) {
                dataSent=true;
                toPachube(2, device.arguments());
            } else {
                strncpy(stowValues[2],device.arguments(),16);
                if (!stowTimes[2]) stowTimes[2]=millis();
            }
            break;
        case _TS4_: 
            if (!throttle) {
                dataSent=true;
                toPachube(3, device.arguments()); 
            } else {
                strncpy(stowValues[3],device.arguments(),16);
                if (!stowTimes[3]) stowTimes[3]=millis();
            }
            break;
        case _TS5_: 
            if (!throttle) {
                dataSent=true;
                toPachube(4, device.arguments()); 
            } else {
                strncpy(stowValues[4],device.arguments(),16);
                if (!stowTimes[4]) stowTimes[4]=millis();
            }
            break;
        case _FAN_: 
            if (!throttle) {
                dataSent=true;
                toPachube(5, device.arguments());
            } else {
                strncpy(stowValues[5],device.arguments(),16);
                if (!stowTimes[5]) stowTimes[5]=millis();
            }
            break;
        case _CHL_: 
            if (!throttle) {
                dataSent=true;
                toPachube(6, device.arguments());
            } else {
                strncpy(stowValues[6],device.arguments(),16);
                if (!stowTimes[6]) stowTimes[6]=millis();
            }
            break;
        case _STC_: 
            char * finger;
            if ((finger=strchr(device.arguments(),' ')) != NULL) {
                finger[0]='\0';
                toPachube(8, finger+1);
            }
            toPachube(7, device.arguments()); break; 
        case _FTL_: toPachube(7, device.arguments()); break;
            break;
        default:
            console.printf("%s",device.line());
            break;
    }
    if (dataSent) {
        throttleMark=millis();
    } else {
        clearStowedValues(); 
    }
}

void clearStowedValues() {
    bool throttle = false;
    if ((millis()-throttleMark<TROTTLE_MILLIS)) { 
        throttle=true;
    }
    
    long unsigned int oldest=millis(); // not rollover proof.
    bool stowed=false;
    uint8_t next=0;
    for (uint8_t i=0; i<7; i++) { // look for the earliest stowed value
        if (stowTimes[i]) {
            if (stowTimes[i]) {
                if (stowTimes[i]<oldest) {
                    oldest=stowTimes[i];
                    stowed=true;
                    next=i;
                }
            }
        }
    }
    if (stowed && !throttle) {
        toPachube(next,stowValues[next]);
        throttleMark=millis();
        stowTimes[next]=0;
    }
}

/*--------------------------------------------------------------------typeFile()
 * type file 
 *----------------------------------------------------------------------------*/

void typeFile(char *fname) {
    SdFile file2;
    int16_t n;
    char nfname[16];
    char buf[65];
    for (n=0; fname[n] && n<15; n++) {
        if (fname[n]=='\r'||fname[n]=='\n') {
            nfname[n]='\0';
        } else if (fname[n]=='.') {
            nfname[n]='.';
        } else {
            nfname[n]=fname[n];//nfname[n]=toupper(fname[n]);
        }
    }
    if (file2.open (&root,nfname,O_READ)) {
		console.printf("SOD:%s\r\n",nfname);
        while ((n = file2.read(buf, sizeof(buf)-1)) > 0) {
            buf[n]='\0';
            console.puts(buf);
        }
		console.printf("EOD:%s\r\n",nfname);
    } else { 
        console.printf("DBG: Couldnt open '%s'\r\n",nfname);        
    }
    
}
#define MAX_INPUT_LINE 100

char inBuffer[MAX_INPUT_LINE];

/*-----------------------------------------------------------------networkScan()
 * scan network for access points
 *----------------------------------------------------------------------------*/
int networkScan() {
    uint8_t index=0;
	char endofinput[]="No.Of AP Found:";
	char nfound[4]="0";
	uint8_t ch='0';
    if (!hasWifiModule) return 0;

	radio.discard();
	radio.printf("\r\n");
	radio.discard();
	radio.printf("\r\nAT+WS\r\n");
	while (!radio.available()); // and some timeout.
	
	while ((index<15)){
		while (radio.available()) {
			console.putChar(ch=(char) radio.read());
			if ((ch=='\n')||(ch=='\r')) index=0;
			if (ch==endofinput[index]) index++;
		}
		
	}
	
	while (!radio.available()); // and some timeout.
	ch='\0'; index=0;
	while ((index<4)&&(ch!='\r')&&(ch!='\n')){
		while(radio.available()) {
			console.putChar(ch=(char) radio.read());
			if (isdigit(ch)) nfound[index++]=ch;
		}
	}
	radio.discard();
	console.printf("\r\n");
	nfound[index]='\0';
	return(atoi(nfound));
	
}

/*-----------------------------------------------------------------isErrorOrOK()
 * parse response from module
 *----------------------------------------------------------------------------*/
int isErrorOrOK(char *line) {
	if (line[0]=='O'&&line[1]=='K'&&line[2]=='\0') return 1;
	if (line[0]=='O'&&line[1]=='K'&&line[2]=='\r') return 1;
	if (line[0]=='E'&&line[1]=='R'&&line[2]=='R'
        &&line[3]=='O'&&line[4]=='R') return -1;

	return 0;
}

/*-------------------------------------------------------------------radioDate()
 * get date from radio module.
 *----------------------------------------------------------------------------*/

time_t radioDate() {
    time_t retTime=0L;
    if (!hasWifiModule) return retTime;
	radio.discard();
	radio.printf("\r\nAT+GETTIME=?\r\n");
	while (!radio.gotLine()); 
	while (!(isErrorOrOK(radio.line()))){
                    //012345678901234567890q123456789012
		if (!strncmp("Current Time in msec since epoch=",radio.line(),32)) {
            console.printf("DBG: epoch as a string=%s\r\n",radio.line()+33);
            retTime=(time_t)(atoll(radio.line()+33)/1000L);
            console.printf("DBG: Epoch=%lu \r\n",retTime);
//            strftime((&retTime));
//            console.printf("DBG: %s\r\n",);

        }
		while (!radio.gotLine());	
	}
	
	radio.discard();
	return(retTime);
	
}

/*-------------------------------------------------------------------toPachube()
 * push data to pachube. 
 *
 * this seems broken in the first section where if the network is 
 * dissassociated
 *----------------------------------------------------------------------------*/

int toPachube(int key, char * value) {
    uint8_t n,index=0;
	int retval=0;
	char databuffer[36];
	char cid[4]="0";
    bool gotResponse=false;
    enum {CONNECT, ERROR, DISASSOCIATED} status=ERROR;//statii;
	uint8_t ch='0';

    if (!hasWifiModule) return retval;

	radio.discard();
	radio.printf("\r\n");
	radio.discard();
	radio.printf("\r\nAT+NCTCP=216.52.233.122,80\r\n");
	while (!radio.available()); // and some timeout.
    
	// ERROR
    // CONNECT <CID>
    // DISASSOCIATED
 	digitalWrite(ORN_LED,LOW);
    
	while (!gotResponse){ // this needs to catch errors.
		while (radio.available()) {
            databuffer[index]=ch=(char) radio.read();
            if (index<36) index++;
			//console.putChar(ch);
			if ((ch=='\n')||(ch=='\r')) {
                databuffer[index]='\0';
                if (strncmp("CONNECT", databuffer,6)==0) {
                    if (index>7) {
                        for (index=0, n=8; index<3 && databuffer[n];) {
                            cid[index++]=databuffer[n++]; // if isdigit????
                        }
                        cid[index]='\0';
                    }
                    status=CONNECT;
                    gotResponse=true;
                    //console.printf("DBG: Connect CID=%s\r\n",cid);
                } else if (strncmp("ERROR", databuffer,5)==0) {
                    gotResponse=true;
                    status=ERROR;
                    
                } else if (strncmp("NOT ASSOCIATED",databuffer,13)==0) {
                    gotResponse=true;
                    status=DISASSOCIATED;
                } else if (strncmp("DISASSOCIATED",databuffer,13)==0) {
                        gotResponse=true;
                        status=DISASSOCIATED;
                }
                index=0;
            }
 		}
		
	}
	digitalWrite(ORN_LED,HIGH);
    
    if (status!=CONNECT) {
        console.printf("DBG: toPachube() NOT CONNECTED! (%d) Exiting\r\n", status);
        radio.discard();
       return(0);
     }
	snprintf(databuffer, 36, "%d,%s",key,value);
	radio.printf("\033S%dPUT /v2/feeds/%s.csv HTTP/1.1\r\n",atoi(cid),pachubeFeed);
	radio.printf("User-Agent: SuspectDevices/Baco-matic5000 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.15\r\n");
	radio.printf("Host: api.pachube.com\r\nAccept: */*\r\nConnection: close\r\n");
	radio.printf("X-PachubeApiKey: %s\r\n",pachubeKey);
	radio.printf("Content-Length: %d\r\n\r\n%s\r\n\033E",strlen(databuffer),databuffer);
	//console.printf("DBG: Sending to feed %s: (%d)%s \r\n",pachubeFeed,strlen(databuffer),databuffer);
    
    //
    // sent data, wait for an OK
    //
	while (!radio.gotLine()); 
	while (!(retval=isErrorOrOK(radio.line()))){
		//console.printf("DBG: %s\r\n",radio.line());
		while (!radio.gotLine()) {
			;	
		}		
	}
    
	if (retval<1) {
        console.printf("DBG: NOT OK! %s\r\n",radio.line());
        radio.discard();
        return(0);
    }
    //
    // should get an escaped stream followed by a disconnect
    //
	index=0;
    gotResponse=false;
	digitalWrite(ORN_LED,LOW);
    
    while (!radio.gotLine());
    sprintf(databuffer,"%c%c%c%c%cS%dHTTP",0x1b,0x4f,0x1b,0x4f,0x1b,atoi(cid)); // the double escapeing I dont get.
    if(!strncmp(databuffer, radio.line(),strlen(databuffer)) ) {
        databuffer[0]=radio.line()[16];
        databuffer[1]=radio.line()[17];
        databuffer[2]=radio.line()[18];
        databuffer[3]='\0';
        //console.printf("DBG:[%d] %s\r\n",retval=atoi(databuffer),radio.line()+7);
    }
    while (!radio.gotLine()); 
    radio.discard();
    delay(500); // this is not so good.
	digitalWrite(ORN_LED,HIGH);
    return(retval);
}

const char months[12][4]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

int monthNumber(char *monthName) {
    int i;
    for (i=0; i<12 && (monthName[0]!=months[i][0] || monthName[1]!=months[i][1] || monthName[2]!=months[i][2]); i++);
    return i+1;
}

/*-------------------------------------------------------------------parseDate()
 * parse date from http convert to format requested by wireless module
 * Date: Sun, 03 Jun 2012 01:22:34 GMT
 * 0123456789012345678901234567890123456
 * AT+SETTIME=<dd/mm/yyyy>,<HH:MM:SS>
 *             0123456789 0 12345678 901234567890123456
 *----------------------------------------------------------------------------*/

void parseDate(char *httpDate, char *targetBuffer) {
    targetBuffer[0]=httpDate[11];
    targetBuffer[1]=httpDate[12];
    targetBuffer[2]='/';
    uint8_t mnth=monthNumber(httpDate+14);
    targetBuffer[3]=(mnth<10)?'0':'1';
    targetBuffer[4]=(mnth%10)+'0';
    targetBuffer[5]='/';
    targetBuffer[6]=httpDate[18];
    targetBuffer[7]=httpDate[19];
    targetBuffer[8]=httpDate[20];
    targetBuffer[9]=httpDate[21];
    targetBuffer[10]=',';
    targetBuffer[11]=httpDate[23];
    targetBuffer[12]=httpDate[24];
    targetBuffer[13]=':';
    targetBuffer[14]=httpDate[26];
    targetBuffer[15]=httpDate[27];
    targetBuffer[16]=':';
    targetBuffer[17]=httpDate[29];
    targetBuffer[18]=httpDate[30];
    targetBuffer[19]='\0';    
}
 
/*-----------------------------------------------------------------pachubeDate()
 * pull date from pachube server
 *----------------------------------------------------------------------------*/
int pachubeDate() {
    uint8_t n,index=0;
	int retval=0;
	char databuffer[36];
	char cid[4]="0";
    bool gotResponse=false;
    enum {CONNECT, ERROR, DISASSOCIATED} status=ERROR;//statii;
	uint8_t ch='0';

    if (!hasWifiModule) return retval;

	radio.discard();
	radio.printf("\r\n");
	radio.discard();
	radio.printf("\r\nAT+NCTCP=216.52.233.122,80\r\n");
	while (!radio.available()); // and some timeout.
    
	// ERROR
    // CONNECT <CID>
    // DISASSOCIATED
 	digitalWrite(ORN_LED,LOW);
    
	while (!gotResponse){ // this needs to catch errors.
		while (radio.available()) {
            databuffer[index]=ch=(char) radio.read();
            if (index<36) index++;
			//console.putChar(ch);
			if ((ch=='\n')||(ch=='\r')) {
                databuffer[index]='\0';
                if (strncmp("CONNECT", databuffer,6)==0) {
                    if (index>7) {
                        for (index=0, n=8; index<3 && databuffer[n];) {
                            cid[index++]=databuffer[n++]; // if isdigit????
                        }
                        cid[index]='\0';
                    }
                    status=CONNECT;
                    gotResponse=true;
                    console.printf("DBG: Connect CID=%s\r\n",cid);
                } else if (strncmp("ERROR", databuffer,5)==0) {
                    gotResponse=true;
                    status=ERROR;
                    
                } else if (strncmp("DISASSOCIATED",databuffer,index)==0) {
                    gotResponse=true;
                    status=DISASSOCIATED;
                }
                index=0;
            }
 		}
		
	}
	digitalWrite(ORN_LED,HIGH);
    

    if (status!=CONNECT) {
        console.printf("DBG: toPachube NOT CONNECTED! (%d) Exiting\r\n", status);
        radio.discard();
        return(0);
    }
	//snprintf(databuffer, 36, "%d,%s",key,value);
	radio.printf("\033S%dHEAD / HTTP/1.1\r\n",atoi(cid),pachubeFeed);
	radio.printf("User-Agent: SuspectDevices/Baco-matic5000 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.15\r\n");
	radio.printf("Host: api.pachube.com\r\nAccept: */*\r\nConnection: close\r\n");
	radio.printf("X-PachubeApiKey: %s\r\n",pachubeKey);
	radio.printf("Content-Length: 0\r\n\r\n\r\n\033E",strlen(databuffer),databuffer);
	//console.printf("DBG: Sending Head / request\r\n");
    
    //
    // sent data then wait for an OK
    //
	while (!radio.gotLine()); 
	while (!(retval=isErrorOrOK(radio.line()))){
		//console.printf("DBG: %s\r\n",radio.line());
		while (!radio.gotLine()) {
			;	
		}		
	}
    
	if (retval<1) {
        console.printf("DBG: NOT OK! %s\r\n",radio.line());
        radio.discard();
        return(0);
    }
    //
    // should get an escaped stream followed by a disconnect
    //
	index=0;
    gotResponse=false;
	digitalWrite(ORN_LED,LOW);
    
    while (!radio.gotLine());
    sprintf(databuffer,"%c%c%c%c%cS%dHTTP",0x1b,0x4f,0x1b,0x4f,0x1b,atoi(cid));
    if(!strncmp(databuffer, radio.line(),strlen(databuffer)) ) {
        databuffer[0]=radio.line()[16];
        databuffer[1]=radio.line()[17];
        databuffer[2]=radio.line()[18];
        databuffer[3]='\0';
        console.printf("DBG:[%d] %s\r\n",retval=atoi(databuffer),radio.line()+7);
    }
    while (!radio.gotLine()); 
    if(!strncmp("Date:",radio.line(),5)) {
        //Date: Sun, 03 Jun 2012 01:22:34 GMT
        //AT+SETTIME=<dd/mm/yyyy>,<HH:MM:SS>
        parseDate(radio.line(),databuffer); 
        radio.printf("AT+SETTIME=%s\r\n",databuffer);
    }	while (!(retval=isErrorOrOK(radio.line()))){
		while (!radio.gotLine()) {
			;	
		}		
	}
    delay(100);
	digitalWrite(ORN_LED,HIGH);
    return(retval);
}

/*-----------------------------------------------------------------networkJoin()
 * join a network...
 *----------------------------------------------------------------------------*/

int networkJoin(char *ssid) {

    int retval=0;
    if (!hasWifiModule) return retval;
    strcpy(networkSSID, ssid);
    console.printf("\r\nDBG: joining: %s\r\n",networkSSID);
    Watchdog_Reset();
	radio.discard();
	radio.printf("\r\nATE0\r\nAT+WA=%s\r\n",ssid);
	while (!radio.gotLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radio.line()))){
		//console.puts(radio.line());
		while (!radio.gotLine()) { // timeout?
			;	
		}		
	}
    Watchdog_Reset();
	radio.printf("\r\nAT+ndhcp=1\r\n");
    delay(1000); // this should be an idle.
	while (!radio.gotLine()); // and some timeout.
	while (!(isErrorOrOK(radio.line()))){
		console.printf("DBG: %s",radio.line());
		while (!radio.gotLine()) {
			;	
		}		
	}
    //console.printf("\r\nDBG: join %s (%d)\r\n",networkSSID,retval=networkStatus());
	return(networkStatus());	
}

/*----------------------------------------------------------networkSetPassword()
 * set network password.
 *----------------------------------------------------------------------------*/

int networkSetPassword(char *pwd) {
    int retval=0;	
    //    strncpy(networkPassword, pwd, MAX_NETWORK_PASSWORD_LENGTH-1);
    strcpy(networkPassword, pwd);
	radio.discard();
	radio.printf("\r\nAT+WWPA=%s\r\n",pwd);
	while (!radio.gotLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radio.line()))){
		//console.puts(radio.line());
		while (!radio.gotLine()) {
			;	
		}
		
	}
    //console.printf("\r\nDBG: Password shoudld be: %s (%d)\r\n",networkPassword,retval);
	return(retval>=1);
	
}
/*---------------------------------------------------------------networkStatus()
 * get network status.
 *----------------------------------------------------------------------------*/

int networkStatus() {
    int retval=0;
    if (!hasWifiModule) return retval;
    radio.discard();
	radio.printf("\r\n");
	radio.discard();
	radio.printf("\r\nAT+WSTATUS\r\n");
	while (!radio.gotLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radio.line()))){
        if ((radio.line()[0]=='N') && (radio.line()[1] == 'O') && (radio.line()[2] == 'T')) {
        //if (strncmp("NOT ASSOCIATED",radio.line(),10) ==0){ //wtf????
            //console.printf("DBG: NOT ASSOCIATED\r\n");
            return(0);
        } else {
//            console.printf("DBG:%s",radio.line());
        }
		while (!radio.gotLine()) {
			// and some timeout!!!
            ;	
		}
	}
	//console.puts(radio.line());
	return(retval>=1);
	
}

/*----------------------------------------------------------handleConsoleInput()
 * handle input from user (console)
 *----------------------------------------------------------------------------*/

void handleConsoleInput() {
    uint8_t retval;
	uint8_t index=0;
	uint8_t ch='0';
	SdFile finger;
    
    switch (console.keyValue()) {
        case _DIR_:
			console.printf("SOD:\r\n");
            root.ls(LS_DATE | LS_SIZE);
 			console.printf("EOD:\r\n");
			break;
        case _LSV_:
			console.printf("LSV:" BOM_VERSION "\r\n");
			break;
        case _TYP_:  
            typeFile(console.arguments());
            break;
        case _NSC_:  
			console.printf("SOD:\r\n");
			retval=networkScan();
			console.printf("EOD:\r\n");
            console.printf("\nDBG: found=%d\r\n",retval);
            break;
        case _NJN_:  
			//console.printf("SOD:\r\n");
			retval=networkJoin(console.arguments());
			//console.printf("EOD:\r\n");
            console.printf("\nDBG: joined=%s\r\n",retval?"TRUE":"FALSE");
            break;
        case _NPW_:  
			//console.printf("SOD:\r\n");
			retval=networkSetPassword(console.arguments());
			//console.printf("EOD:\r\n");
            console.printf("\nDBG: pwd set=%s\r\n",retval?"TRUE":"FALSE");
            break;
        case _NST_:  
			retval=networkStatus();
            console.printf("NST: %s\r\n",retval?"CONNECTED":"NOT CONNECTED");
            break;
		case _FMT_: // there really should be some REALLY do you mean this here but.....
			root.openRoot(&volume);
			if (finger.open(&root, ".", O_WRITE|O_READ)) {
				console.printf("\nDBG: Opened / \r\n");
				finger.rmRfStar();
			} else {
				console.printf("\nDBG: FAIL \r\n");
			}	
			break;
            
		case _TPT_:
			toPachube(1, console.arguments());
			break;
        case _TX2_:
            radio.printf("%s",console.arguments());
            index=0;
            // delay(1000);
            while (radio.available()) {
                inBuffer[index++]=ch=radio.read();
				if( index>= 99 || ((ch== '\n') || ch !='\r')) {
					inBuffer[index]='\0';
					console.puts(inBuffer);
					index=0; 
					delay(100);
				}
            } 
            inBuffer[index]='\0';
            console.puts(inBuffer);
			console.puts((char *) "\r\n");
            break;
// set to one manually test output for.
#if 0
        case _TS1_: toPachube(0, console.arguments()); break;
        case _TS2_: toPachube(1, console.arguments()); break;
        case _TS3_: toPachube(2, console.arguments()); break;
        case _TS4_: toPachube(3, console.arguments()); break;
        case _TS5_: toPachube(4, console.arguments()); break;
        case _FAN_: toPachube(5, console.arguments()); break;
        case _CHL_: toPachube(6, console.arguments()); break;
        case _STC_: toPachube(7, console.arguments()); break;
#endif			

        case _PKY_:
            strncpy(pachubeKey, console.arguments(), MAX_PATCHUBE_KEY_LENGHT-1);
            stripcrlf(pachubeKey);
            break;
        case _PFD_:
            strncpy(pachubeFeed, console.arguments(), MAX_PATCHUBE_FEED_LENGHT-1);
            stripcrlf(pachubeFeed);
            break;
        case _SGT_ :
            readSettings();
            break;
        case _SSV_ :
            writeSettings();
            break;
        case _TFD_ :
            ; writeSettings();
            break;
        case _TFN_ :
            pachubeDate();
            break;
            
        case _LTM_ :
            radioDate(); //do something to format me....
            break;
        case _SYN_ :
            console.printf("ACK: BOM5k\r\n");
            break;
        case _SSN_:                
            if (deviceIsConnected) {
                // ask the device....
            } else { 
                console.printf("SSN:%s\r\n",deviceSSN);   
            }
            break;

        case _SWV_:                
            if (deviceIsConnected) {
                // ask the device....
            } else { 
                console.printf("SWV:%s\r\n",deviceSWV);   
            }
            break;
        case _HWV_:                
            if (deviceIsConnected) {
                // ask the device....
            } else { 
                console.printf("HWV:%s\r\n",deviceHWV);   
            }
            break;
            
        default:
            console.printf("DBG: forwarding (%s) to device\r\n",console.key());
            device.puts(console.line());
            //device.puts("\r\n");
            break;
    }
    
    
}

/*----------------------------------------------------------------readSettings()
 * read settings file
 *----------------------------------------------------------------------------*/

void readSettings() {
	int n,cc;
	char ch, buff[24];
	char  buff2[80];
	unsigned int ndx=0;
	int read=0;
    if (!settingsFile.open(&root, settingsFileName, O_READ)) {
        console.printf("DBG: Creating New Settings File\r\n"); console.flush();
        writeSettings();
        return;
	}
    console.printf("DBG: Opened Settings File: %s\r\n",settingsFileName);
	settingsFile.seekSet(0);
	while ((read=settingsFile.read(buff, sizeof(buff))) > 0) {
        //console.printf("DBG: READ %d\r\n",read); console.flush();
        for (cc=0; cc < read ; cc++) {
            ch=buff[cc];
            //console.putChar(ch); console.flush();
            buff2[ndx]=ch;
            if (ndx<MAX_INPUT_LINE) {ndx++;}
            if (ch=='\n'){//console.putChar('\r');
                if(ndx) {
                    ndx--; // eat the newline.
                }
                buff2[ndx]='\0';
                //console.puts(buff2);
                if ((ndx>4) && buff2[3]=='!') {//fix this when the files are better
                    buff2[3]='\0';
                    n=lookupIndex(buff2);
                    switch (n) {
                        case _NPW_:
                            networkSetPassword(buff2+4);
                            break;
                        case _NJN_:
                            Watchdog_Reset();
                            networkJoin(buff2 +4);
                            break;
                        case _PKY_:
                            strncpy(pachubeKey, buff2+4, MAX_PATCHUBE_KEY_LENGHT-1);
                            break;
                        case _PFD_:
                            strncpy(pachubeFeed, buff2+4, MAX_PATCHUBE_FEED_LENGHT-1);
                            break;
                        default:
                            break;
                    }
                    
                }
            
                ndx=0; 

            }
        }
    }
    
    console.printf("DBG: Finished with Settings File: %s\r\n",settingsFileName);
    
	settingsFile.close();
}
/*---------------------------------------------------------------writeSettings()
 * write settings file
 *----------------------------------------------------------------------------*/

void writeSysInfoFile() {

    char wrtBuffer[80];
    
    if (!sysinfoFile.open(&root, sysinfoFileName, O_WRITE|O_CREAT)) {
        // bitch.... ();
        return;
	}
    console.printf("DBG: Opened Settings File: %s\r\n",sysinfoFileName);
    snprintf(wrtBuffer,80,"SSN!%s\n",networkPassword);
    sysinfoFile.write(wrtBuffer,strlen(wrtBuffer));
    snprintf(wrtBuffer,80,"HWV!%s\n",networkSSID);
    sysinfoFile.write(wrtBuffer,strlen(wrtBuffer));
    snprintf(wrtBuffer,80,"SWV!%s\n",pachubeKey);
    sysinfoFile.write(wrtBuffer,strlen(wrtBuffer));
    snprintf(wrtBuffer,80,"PFD!%s\n",pachubeFeed);
    sysinfoFile.write(wrtBuffer,strlen(wrtBuffer));
    sysinfoFile.close();
    console.printf("DBG: Closing Settings File: %s\r\n",sysinfoFileName);
    
    
}

/*-------------------------------------------------------------readSysInfoFile()
 * read sysinfo file
 *----------------------------------------------------------------------------*/

void readSysInfoFile() {
	int n,cc;
	char ch, buff[24];
	char  buff2[80];
	unsigned int ndx=0;
	int read=0;
    if (!sysinfoFile.open(&root, sysinfoFileName, O_READ)) {
        console.printf("DBG: Creating New Sys Info File\r\n"); console.flush();
        writeSysInfoFile();
        return;
	}
    console.printf("DBG: Opened Sys Info File: %s\r\n",sysinfoFileName);
	sysinfoFile.seekSet(0);
	while ((read=sysinfoFile.read(buff, sizeof(buff))) > 0) {
        //console.printf("DBG: READ %d\r\n",read); console.flush();
        for (cc=0; cc < read ; cc++) {
            ch=buff[cc];
            //console.putChar(ch); console.flush();
            buff2[ndx]=ch;
            if (ndx<MAX_INPUT_LINE) {ndx++;}
            if (ch=='\n'){//console.putChar('\r');
                if(ndx) {
                    ndx--; // eat the newline.
                }
                buff2[ndx]='\0';
                //console.puts(buff2);
                if ((ndx>4) && buff2[3]=='!') {
                    buff2[3]='\0';
                    n=lookupIndex(buff2);
                    switch (n) {
                        case _SSN_:
                            strncpy(deviceSSN, buff2+4, MAX_SSN_LENGTH - 1);
                            break;
                        case _HWV_:
                            strncpy(deviceHWV, buff2+4, MAX_HWV_LENGTH - 1);
                            break;
                        case _SWV_:
                            strncpy(deviceSWV, buff2+4, MAX_SWV_LENGTH - 1);
                            break;
                        //case _DCA_: // disconnected at.
                        //    strncpy(pachubeFeed, buff2+4, MAX_PATCHUBE_FEED_LENGHT-1);
                        //    break;
                        default:
                            break;
                    }
                    
                }
                
                ndx=0; 
                
            }
        }
    }
    
    console.printf("DBG: Finished with Sys Info File: %s\r\n",sysinfoFileName);
    
	sysinfoFile.close();
}
/*---------------------------------------------------------------writeSettings()
 * write settings file
 *----------------------------------------------------------------------------*/

void writeSettings() {
    
    char wrtBuffer[80];
    
    if (!settingsFile.open(&root, settingsFileName, O_WRITE|O_CREAT)) {
        // bitch.... ();
        return;
	}
    console.printf("DBG: Opened Settings File: %s\r\n",settingsFileName);
    snprintf(wrtBuffer,80,"NPW!%s\n",networkPassword);
    settingsFile.write(wrtBuffer,strlen(wrtBuffer));
    snprintf(wrtBuffer,80,"NJN!%s\n",networkSSID);
    settingsFile.write(wrtBuffer,strlen(wrtBuffer));
    snprintf(wrtBuffer,80,"PKY!%s\n",pachubeKey);
    settingsFile.write(wrtBuffer,strlen(wrtBuffer));
    snprintf(wrtBuffer,80,"PFD!%s\n",pachubeFeed);
    settingsFile.write(wrtBuffer,strlen(wrtBuffer));
    settingsFile.close();
    console.printf("DBG: Closing Settings File: %s\r\n",settingsFileName);
    
    
}

/*-----------------------------------------------------------------lookupIndex()
 * look up index based on 3 letter key
 *----------------------------------------------------------------------------*/

int lookupIndex(char *key) {
    int ndx=0;
    int ptr=0;
    while (ndx<NKEYWORDS) {
        if (   (keywords[ptr]==toupper(key[0])) 
            && (keywords[ptr+1]==toupper(key[1])) 
            && (keywords[ptr+2]==toupper(key[2]))
            ) { 
            return ndx;
        } else {
            ptr+=3;
            ndx++;
        }
    }
    return -1;
}
/*------------------------------------------------------------setupRadioModule()
 * 
 *----------------------------------------------------------------------------*/
void setupRadioModule(void) {
    long int mark=millis();
	radio.printf("\r\n");
	radio.discard();
    radio.printf("ATE0\r\nAT&K0\r\nAT&R0\r\nAT+WD\r\nAT+NCLOSEALL\r\n");
    //radio.printf("ATE0\r\nATH\r\n");
	while (!radio.gotLine()); 
	while ((millis()-mark<1000)&&(!(hasWifiModule=isErrorOrOK(radio.line())))){
		//console.printf("DBG: %s\r\n",radio.line());
		while (!radio.gotLine()) {
			;	
		}		
	}
    radio.discard();
}

bool pingDevice() {
    // device.discard();
    // device.printf("\r\n\r\nSYN!\r\n");
    // delay(100);
    // if(device.gotline(500)) {
    //   return (!strcmp(device.key,"ACK")) // or just return true
    //
}

/*-----------------------------------------------------------------------setup()
 * Setup needs to check for module.
 * 
 *----------------------------------------------------------------------------*/

#define LINE_BUFFER_MAX 80
void setup() {

    int logNo;
	char configLineBuffer[LINE_BUFFER_MAX];
    spi.begin(SPI_281_250KHZ, MSBFIRST, 0);
	
	pinMode(GRN_LED,OUTPUT);
	pinMode(ORN_LED,OUTPUT);
	pinMode(RED_LED,OUTPUT);
	digitalWrite(GRN_LED,HIGH);
	digitalWrite(ORN_LED,LOW);
	digitalWrite(RED_LED,LOW);

    iwdg_init(IWDG_PRE_256, WATCHDOG_TIMEOUT);
    Watchdog_Reset();

	
    if (!card.init(&spi)) { 
    //if (!card.init()) { 
        console.printf("FTL: card.init failed\r\n");
    }
    delay(100);
    
    // initialize a FAT volume
    if (!volume.init(&card)) {
        console.printf("FTL: volume.init failed\r\n");
    }
    
    // open the root directory
    if (!root.openRoot(&volume)) 
        console.printf("FTL: openRoot failed\r\n");
#ifdef REMOVE_ALL_ZERO_LENGTH_FILES
    console.printf("DBG: removing zero length files\r\n");

    
    for (logNo=0; logNo<512; logNo++) {
        Watchdog_Reset();
        togglePin(ORN_LED);
        togglePin(GRN_LED);
        if (!(logNo%64)) {
            console.printf("\r\nDBG: (%d)",logNo);
        } 
        
        snprintf(logFileName,15,"LOG%03d.TXT",logNo);
        if (file.open(&root, logFileName, O_READ)) {
            if(file.seekSet(40)) {
                console.putChar('.');
                file.close();
            } else {
                console.putChar('x');
                file.close();
                file.remove(&root,logFileName);
            }
        }
    }
    console.puts("\r\n");
#endif    
    /*
     * find a new file to open
     * in development the logger is rebooted multiple times. 
     * This leads to hundreds of zero length files so we delete any we find.
     */
    for (logNo=0; (!logOpened) && logNo<512; logNo++) {
        Watchdog_Reset();
        togglePin(ORN_LED);
        togglePin(GRN_LED);
        
        if (!(logNo%64)) {
            console.printf("\r\nDBG: (%0.3d)",logNo);
        } //int snprintf(char *str, size_t size, const char *format, ...);
        
        snprintf(logFileName,15,"LOG%03d.TXT",logNo);
        if (file.open(&root, logFileName, O_READ)) {
            if(file.seekSet(4)) {
                //SerialUSB.print("DBG: Exists  :"); SerialUSB.println(logFileName);
                file.close();
            }  else {
                console.putChar('x');
                file.close();
                file.remove(&root,logFileName);
                if (file.open(&root, logFileName, O_CREAT|O_READ|O_WRITE)) {
                    //SerialUSB.print("DBG: New File:"); SerialUSB.println(logFileName); 
                    logOpened=true;
                    file.sync();
                    file.close();
                    file.open(&root,logFileName,O_WRITE|O_READ);
                }
            }
                           
        } else if (file.open(&root, logFileName, O_CREAT|O_READ|O_WRITE)) {
            //SerialUSB.print("DBG: New File:"); SerialUSB.println(logFileName); 
            logOpened=true;
            file.sync();
            file.close();
            file.open(&root,logFileName,O_WRITE|O_READ);
			while (file.read(configLineBuffer,LINE_BUFFER_MAX)) {
				
			}
            //file.write("\r\n",2);
            file.sync();
        }    
    }
    
    console.puts("\r\n");
    //if (!logOpened) SerialUSB.println("FTL: openRoot failed");

    setupRadioModule();
    
	digitalWrite(GRN_LED,LOW);
	digitalWrite(RED_LED,HIGH);
    readSettings();
	console.printf("LSV:" BOM_VERSION "\r\n");
    console.printf("NST: %s\r\n",networkStatus()?"CONNECTED":"NOT CONNECTED");
	digitalWrite(ORN_LED,HIGH);
	digitalWrite(RED_LED,networkStatus()?HIGH:LOW);
	
}
volatile long int lastCheck;
volatile long int lastConnected;

/*------------------------------------------------------------------------loop()
 * 
 *----------------------------------------------------------------------------*/

void loop() {
    static int disconnected;
    if (millis() - lastCheck > 3000) {
        digitalWrite(RED_LED,LED_OFF);
        if ((! hasWifiModule) || networkStatus()) {
            digitalWrite(RED_LED,LED_OFF);
            disconnected=0;
        } else {
            digitalWrite(RED_LED,LED_ON);
            if (++disconnected > 10) {
                char tempssid[40];
                strcpy(tempssid,networkSSID);
                digitalWrite(RED_LED,networkJoin(tempssid)?LED_OFF:LED_ON);
            }
        }
        digitalWrite(GRN_LED,SerialUSB.isConnected()?HIGH:LOW);
        if (!deviceIsConnected) {
            deviceIsConnected=pingDevice();
        }
        lastCheck=millis();
    }
        
    while (device.gotLine()) {
        handleDeviceInput();
        deviceIsConnected=true;
        //deviceLastContact=time();
    }
    
    while (console.gotLine()) {
        digitalWrite(GRN_LED,LOW);
        handleConsoleInput();
        digitalWrite(GRN_LED,HIGH);
    }
    clearStowedValues();
    Watchdog_Reset();

}


