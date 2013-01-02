/*------------------------------------------------------------datalogger/main.cpp
 *
 * This is a functional prototype of a datalogger. 
 * 
 *
 * 
 * Loose framework (thinking out loud). 
 *
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
//#define  Watchdog_Reset iwdg_feed
#define  Watchdog_Reset()


#define BOM_VERSION "BOM5K_"__DATE__

#define KEYWORDS \
    "SYN" "ACK" "NAK" "NOP"

enum keywordIndex {
    _SYN_,_ACK_,_NAK_,_NOP_
};

// use enum to determine the size of the keyword arrays.
#define NKEYWORDS  _NOP_ + 1 
//NKEYWORDS;
#define LOGGERKEYWORDS \
    "NSC"         "NJN" "NPW" "NST" "TPT" "TX2" "PKY"

enum loggerKeywordIndex {
    _NSC_=_NOP_+1,_NJN_,_NPW_,_NST_,_TPT_,_TX2_,_PKY_,
};

//int patchubeIndex[]={_TS1_,_TS2_,_TS3_,_TS4_,_TS5_,_FAN_,_CHL_,_STC_};

#undef NKEYWORDS
#define NKEYWORDS  _PKY_ + 1 
/*-----------------------------------------------------------------DECLARATIONS
 * 
 *----------------------------------------------------------------------------*/

int toTheEthers(int, char *);
int networkStatus();
const char keywords[((NKEYWORDS)*3)+1] = KEYWORDS LOGGERKEYWORDS;

int lookupIndex(char *);
void logData(char *);
void stripcrlf(char *);

/*-------------------------------------------------------------GLOBAL VARIABLES
 * 
 *----------------------------------------------------------------------------*/

HardwareSPI spi(1);
bool connectedToNet=false;
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


char networkPassword[MAX_NETWORK_PASSWORD_LENGTH]="B00B00B00!";
char networkSSID[MAX_NETWORK_SSID_LENGTH]="MuffMuffMuff";
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



void clearStowedValues();

volatile long unsigned int throttleMark;

char stowValues[7][16];
long unsigned int stowTimes[7];
#define TROTTLE_MILLIS 2000


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
        toTheEthers(next,stowValues[next]);
        throttleMark=millis();
        stowTimes[next]=0;
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
        }
		while (!radio.gotLine());	
	}
	
	radio.discard();
	return(retTime);
	
}

/*-----------------------------------------------------------------toTheEthers()
 * push data to stephenWrightsLightSwitch. 
 *
 * this seems broken in the first section where if the network is 
 * dissassociated
 *----------------------------------------------------------------------------*/

int toTheEthers(int key, char * value) {
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
	radio.printf("\r\nAT+NCTCP=198.202.31.182,80\r\n");
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
	//snprintf(databuffer, 36, "%d,%s",key,value);
	
    radio.printf("\033S%dGET /art2013/swls/&state=%s HTTP/1.1\r\n",atoi(cid),value?"ON" : "OFF");
	//radio.printf("\033S%dPUT /art2013/swls/%s.csv HTTP/1.1\r\n",atoi(cid));
	radio.printf("User-Agent: SuspectDevices/Baco-matic5000 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.15\r\n");
	radio.printf("Host: suspectdevices.com\r\nAccept: */*\r\nConnection: close\r\n");
	radio.printf("X-PachubeApiKey: %s\r\n\r\n",pachubeKey);
	//radio.printf("Content-Length: %d\r\n\r\n%s\r\n\033E",strlen(databuffer),databuffer);
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
    switch (console.keyValue()) {
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
            
		case _TPT_:
			toTheEthers(1, console.arguments());
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
        case _PKY_:
            strncpy(pachubeKey, console.arguments(), MAX_PATCHUBE_KEY_LENGHT-1);
            stripcrlf(pachubeKey);
            break;
        default:
            console.printf("DBG: forwarding (%s) to device\r\n",console.key());
            //device.puts("\r\n");
            break;
    }
    
    
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
    //radio.printf("ATE0\r\nAT&K0\r\nAT&R0\r\nAT+WD\r\nAT+NCLOSEALL\r\n");
    radio.printf("ATE0\r\nATH\r\n");
	while (!radio.gotLine()); 
	while ((millis()-mark<1000)&&(!(hasWifiModule=isErrorOrOK(radio.line())))){
		console.printf("DBG: %s\r\n",radio.line());
		while (!radio.gotLine()) {
			;	
		}		
	}
    radio.discard();
    networkSetPassword(networkPassword);
    Watchdog_Reset();
    networkJoin(networkSSID);

}


/*-----------------------------------------------------------------------setup()
 * Setup needs to check for module.
 * 
 *----------------------------------------------------------------------------*/

#define LINE_BUFFER_MAX 80
void setup() {

    //spi.begin(SPI_281_250KHZ, MSBFIRST, 0);
	
	pinMode(GRN_LED,OUTPUT);
	pinMode(ORN_LED,OUTPUT);
	pinMode(RED_LED,OUTPUT);
	digitalWrite(GRN_LED,HIGH);
	digitalWrite(ORN_LED,LOW);
	digitalWrite(RED_LED,LOW);

    //iwdg_init(IWDG_PRE_256, WATCHDOG_TIMEOUT);
    Watchdog_Reset();
    delay(3000);
	console.printf("HELLO?\r\n");
	
    setupRadioModule();
    
	digitalWrite(GRN_LED,LOW);
	digitalWrite(RED_LED,HIGH);
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
	console.printf("HELLO?\r\n");
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
        lastCheck=millis();
    }
        
    
    while (console.gotLine()) {
        digitalWrite(GRN_LED,LOW);
        handleConsoleInput();
        digitalWrite(GRN_LED,HIGH);
    }
    clearStowedValues();
    Watchdog_Reset();

}


// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}

int main(void) {
	console.printf("HELLO?\r\n");
    setup();
	console.printf("HELLO?\r\n");
    while (true) {

        loop();
    }
    
    return 0;
    
}

