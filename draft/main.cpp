/*------------------------------------------------------------datalogger/main.cpp
 *
 *
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
 * TYP[?  ] type file
 * DEL[ ! ] delete file
 * DDS[?  ] dates from logs.
 * DMP[ ! ] dump all logs [in order]
 * 
 * LOG[ !:] log data to current log file
 * LSV[? :] logger software version
 * LTS[ ! ] sync time with client
 * LNT[ ! ] sync time from network.
 *
 * NSC[?  ] network scan
 * NPW[ ! ] network password
 * NJN[?!:] join network.
 *
 * SSV[!  ] save settings (read	 bom5k.set)
 * SGT[?  ] get settings  (write bom5k.set)
 * 
 *-----------------------------------------------------------------------------*/
#include "wirish.h"
#include "iwdg.h"
#include <ctype.h>
#include <SdFat.h>
#include <SdFat.h>
#include <time.h>
#include <RTClock.h>
//#include <EEPROM.h>
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

#define GRN_LED 19
#define ORN_LED 18
#define RED_LED 14
#define LED_ON  LOW
#define LED_OFF HIGH

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
    "DIR"         "DEL" "TX2" "TYP" "NSC" "NJN" "NPW" "NST" "TPT" "FMT" "PKY" \
    "PFD" "PTO" "PIP" "PON" "POF" "SSV" "SGT" "LSV"

enum loggerKeywordIndex {
    _DIR_=_NOP_+1,_DEL_,_TX2_,_TYP_,_NSC_,_NJN_,_NPW_,_NST_,_TPT_,_FMT_,_PKY_,
    _PFD_,_PTO_,_PIP_,_PON_,_POF_,_SSV_,_SGT_,_LSV_
};

//int patchubeIndex[]={_TS1_,_TS2_,_TS3_,_TS4_,_TS5_,_FAN_,_CHL_,_STC_};

#undef NKEYWORDS
#define NKEYWORDS  _LSV_ + 1 

int toPachube(int, char *);
void readSettings(void);
void writeSettings(void);
int networkStatus();
const char keywords[((NKEYWORDS)*3)+1] = KEYWORDS LOGGERKEYWORDS;

int lookupIndex(char *);
void logData(char *);
void stripcrlf(char *);
void stripcrlf(char *mystring) {
    char * finger;
    if ((finger=strchr(mystring,'\r')) != NULL) {
        finger[0]='\0';
    }
    if ((finger=strchr(mystring,'\n')) != NULL) {
        finger[0]='\0';
    }
}
#define MAX_RETURN_VALUE 104

#define WATCHDOG_TIMEOUT 16000
#define  Watchdog_Reset iwdg_feed

HardwareSPI spi(1);
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;
SdFile settingsFile;
char logFileName[16];
char settingsFileName[16]="BOM5K.SET";

bool logOpened=false;

#define MAX_PATCHUBE_KEY_LENGHT 80
#define MAX_PATCHUBE_FEED_LENGHT 10
#define MAX_NETWORK_PASSWORD_LENGTH 40
#define MAX_NETWORK_SSID_LENGTH 64

char networkPassword[MAX_NETWORK_PASSWORD_LENGTH];
char networkSSID[MAX_NETWORK_SSID_LENGTH];
char pachubeKey[MAX_PATCHUBE_KEY_LENGHT]="J_4LnoRNleWbZDm9FYp3XevLWXuSzlDiJwCsQnc1TGFQOUZLMHM9";
char pachubeFeed[MAX_PATCHUBE_FEED_LENGHT]="50019";

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

void logData(char *data) {
    int n;
    n=file.write(data,strlen(data));
    console.printf("DBG: Logging (%d)'%s'",n,data);
    file.sync();
}
volatile long unsigned int throttleMark;
void handleDeviceInput() {
    bool throttle = false;
    bool dataSent = false;
    if ((millis()-throttleMark<1500)) {
        throttle=true;
    }
    switch (device.keyValue()) {
        case _LOG_:
            logData(device.arguments());                
            break;
        case _TS1_: 
            if (!throttle) {
                dataSent=true;
                toPachube(0, device.arguments());
            } break;
        case _TS2_: 
            if (!throttle) {
                dataSent=true;
                toPachube(1, device.arguments());
            } break;
        case _TS3_: 
            if (!throttle) {
                dataSent=true;
                toPachube(2, device.arguments());
            } break;
        case _TS4_: 
            if (!throttle) {
                dataSent=true;
                toPachube(3, device.arguments()); 
            } break;
        case _TS5_: 
            if (!throttle) {
                dataSent=true;
                toPachube(4, device.arguments()); 
            } break;
        case _FAN_: 
            if (!throttle) {
                dataSent=true;
                toPachube(5, device.arguments());
            } break;
        case _CHL_: 
            if (!throttle) {
                dataSent=true;
                toPachube(6, device.arguments());
            } break;
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
        if (dataSent) throttleMark=millis();
}


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

int networkScan() {
    uint8_t index=0;
	char endofinput[]="No.Of AP Found:";
	char nfound[4]="0";
	uint8_t ch='0';
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

int isErrorOrOK(char *line) {
	if (line[0]=='O'&&line[1]=='K'&&line[2]=='\0') return 1;
	if (line[0]=='O'&&line[1]=='K'&&line[2]=='\r') return 1;
	if (line[0]=='E'&&line[1]=='R'&&line[2]=='R'&&line[3]=='O'&&line[4]=='R'&&line[5]=='\0') return -1;
	if (line[0]=='E'&&line[1]=='R'&&line[2]=='R'&&line[3]=='O'&&line[4]=='R'&&line[5]=='\r') return -1;
	return 0;
}


int toPachube(int key, char * value) {
    uint8_t n,index=0;
	int retval;
	char databuffer[36];
	char cid[4]="0";
    bool gotResponse=false;
    enum {CONNECT, ERROR, DISASSOCIATED} status=ERROR;//statii;
	uint8_t ch='0';
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
			console.putChar(ch);
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
                } else if (strcmp("ERROR", databuffer)==0) {
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
        console.printf("DBG: NOT CONNECTED! (%d) Exiting\r\n", status);
        return(0);
    }
	radio.discard();
	snprintf(databuffer, 36, "%d,%s",key,value);
	radio.printf("\033S%dPUT /v2/feeds/%s.csv HTTP/1.1\r\n",atoi(cid),pachubeFeed);
	radio.printf("User-Agent: SuspectDevices/Baco-matic5000 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.15\r\n");
	radio.printf("Host: api.pachube.com\r\nAccept: */*\r\nConnection: close\r\n");
	radio.printf("X-PachubeApiKey: %s\r\n",pachubeKey);
	radio.printf("Content-Length: %d\r\n\r\n%s\r\n\033E",strlen(databuffer),databuffer);
	console.printf("DBG: Sending to feed %s: (%d)%s \r\n",pachubeFeed,strlen(databuffer),databuffer);
    
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
    radio.discard();
    delay(500);
	digitalWrite(ORN_LED,HIGH);
    return(retval);
}

int networkJoin(char *ssid) {

    int retval=0;

    strcpy(networkSSID, ssid);

	radio.discard();
	radio.printf("\r\nATE0\r\nAT+WA=%s\r\n",ssid);
	while (!radio.gotLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radio.line()))){
		//console.puts(radio.line());
		while (!radio.gotLine()) { // timeout?
			;	
		}		
	}
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

int networkStatus() {
    int retval=0;
    radio.discard();
	radio.printf("\r\n");
	radio.discard();
	radio.printf("\r\nAT+WSTATUS\r\n");
	while (!radio.gotLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radio.line()))){
        if ((radio.line()[0]=='N') && (radio.line()[1] == 'O') && (radio.line()[2] == 'T')) {
        //if (strcmp("NOT ASSOCIATED",radio.line()) ==0){ //wtf????
            console.printf("DBG: NOT ASSOCIATED\r\n");
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
// set to one to test output for.
#if 1
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
            
        default:
            console.printf("DBG: forwarding (%s) to device\r\n",console.key());
            device.puts(console.line());
            //device.puts("\r\n");
            break;
    }
    
    
}

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
/*
 * need to find a better way to handle errors if console is connected.
 */
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
        console.printf("FTL: card.init failed");
    }
    delay(100);
    
    // initialize a FAT volume
    if (!volume.init(&card)) {
        console.printf("FTL: volume.init failed");
    }
    
    // open the root directory
    if (!root.openRoot(&volume)) 
        ;//SerialUSB.println("FTL: openRoot failed");
    
    for (logNo=0; (!logOpened) && logNo<512; logNo++) {
        Watchdog_Reset();
       //int snprintf(char *str, size_t size, const char *format, ...);
        snprintf(logFileName,15,"LOG%03d.TXT",logNo);
        if (file.open(&root, logFileName, O_READ)) {
            //SerialUSB.print("DBG: Exists  :"); SerialUSB.println(logFileName);
            file.close();
        } else if (file.open(&root, logFileName, O_CREAT|O_READ|O_WRITE)) {
            //SerialUSB.print("DBG: New File:"); SerialUSB.println(logFileName); 
            logOpened=true;
            file.sync();
            file.close();
            file.open(&root,logFileName,O_WRITE|O_READ);
			while (file.read(configLineBuffer,LINE_BUFFER_MAX)) {
				
			}
            file.sync();
        }    
    }
    //if (!logOpened) SerialUSB.println("FTL: openRoot failed");

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

void loop() {
    static int disconnected;
    if (millis() - lastCheck > 3000) {
        
        if (networkStatus()) {
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
        lastCheck=millis();
    }
        
    while (device.gotLine()) {
        handleDeviceInput();
    }
    while (console.gotLine()) {
        digitalWrite(GRN_LED,LOW);
        handleConsoleInput();
        digitalWrite(GRN_LED,HIGH);
    }
    
    Watchdog_Reset();

}

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
