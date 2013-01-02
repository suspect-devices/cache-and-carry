/******************************************************************************
 *  wifiHack.cpp
 *  Project: datalogger
 *
 *  Created by Donald D Davis on 7/6/12.
 *
 *  The MIT License
 *
 *  Copyright 2012 Suspect Devices. 
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/


#include "wifiHack.h"
char networkPassword[MAX_NETWORK_PASSWORD_LENGTH]="Cut1T0ut";
char networkSSID[MAX_NETWORK_SSID_LENGTH]="SWLS";
char pachubeKey[MAX_PATCHUBE_KEY_LENGHT]="J_4LnoRNleWbZDm9FYp3XevLWXuSzlDiJwCsQnc1TGFQOUZLMHM9";
char pachubeFeed[MAX_PATCHUBE_FEED_LENGHT]="50019";
char pachubeIP[MAX_IP_LENGTH]="216.52.233.122";
bool hasWifiModule=true;
char radioInBuffer[128];
int radioIndex=0;
char radioOutBuffer[128];

#define radio Serial2

void toRadio(const char *format, ...){
    va_list args;
    va_start( args, format );
    vsprintf(radioOutBuffer, format, args );
    usart_tx(USART2, (uint8_t *)radioOutBuffer, strlen(radioOutBuffer));
    usart_tx(USART2, (uint8_t *)"\r\n", 2);
    va_end(args);
}

void discardResponse() { while(radio.available()) radio.read(); radioIndex=0;}


bool gotRadioLine() { //this should be rethought ...
    unsigned int timeout=5000;
    char ch=0;
    bool retval=false;
	unsigned long int mark=millis();
    if(radio.available()){
        while( (millis()-mark < timeout) && radio.available() && radioIndex < 127 && ((ch=radio.read()) != '\n') && ch !='\r' ) {
            radioInBuffer[radioIndex++] = ch;
        }
        if (radioIndex && ((ch=='\n')||(ch=='\r'))) { // should probably check if line is at least 4 chars.
            //radioInBuffer[radioIndex++]='\r';
            //radioInBuffer[radioIndex++]='\n';
            radioInBuffer[radioIndex]='\0';
            radioIndex=0;
            retval=true;
        }
    }
    toConsole(radioInBuffer);
    return true;
}

/*-----------------------------------------------------------------networkScan()
 * scan network for access points
 * seriously rewrite this mr clever jerk.
 *----------------------------------------------------------------------------*/
uint8_t NSCaction(uint8_t source){

//int networkScan() {
    uint8_t index=0;
	char endofinput[]="No.Of AP Found:";
	char nfound[4]="0";
    char outBuffer[100];
    int outdex=0;
	uint8_t ch='0';
    if (!hasWifiModule) return 0;

    toConsole("SOD: Network Scan");
	discardResponse();
	toRadio("\r\n");
	discardResponse();
	toRadio("\r\nAT+WS\r\n");
	while (!radio.available()); // and some timeout.
	
	while ((index<15)){
		while (radio.available()) {
            outBuffer[outdex++]=ch=(char) radio.read();
            if ((ch=='\n')||(ch=='\r')) {
                outBuffer[outdex]='\0';
                toConsole(outBuffer);
                outdex=index=0;
            }
			if (ch==endofinput[index]) index++;
		}
		
	}
	
	while (!radio.available()); // and some timeout.
	ch='\0'; index=0;
	while ((index<4)&&(ch!='\r')&&(ch!='\n')){
		while(radio.available()) {
			//console.putChar(ch=(char) radio.read());
			if (isdigit(ch)) nfound[index++]=ch;
		}
	}
	discardResponse();
	nfound[index]='\0';
    toConsole("%s APs Found",nfound);
    toConsole("EOD: Network Scan");

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
	discardResponse();
	toRadio("\r\nAT+GETTIME=?\r\n");
	while (!gotRadioLine()); 
	while (!(isErrorOrOK(radioInBuffer))){
        //012345678901234567890q123456789012
		if (!strncmp("Current Time in msec since epoch=",radioInBuffer,32)) {
//            toConsole("epoch as a string=%s\r\n",radioInBuffer+33);
            epoch=retTime=(time_t)(atoll(radioInBuffer+33)/1000L);
//            toConsole("Epoch=%lu \r\n",retTime);
//            toConsole("%s\r\n",asctime(gmtime(&retTime)));
            //            toConsole("%s\r\n",);
            
        }
		while (!gotRadioLine());	
	}
	
	discardResponse();
	return(retTime);
	
}

/*-------------------------------------------------------------------toPachube()
 * push data to pachube. 
 *
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
    
	discardResponse();
	toRadio("\r\n");
	discardResponse();
	toRadio("\r\nAT+NCTCP=216.52.233.122,80\r\n");
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
                    //toConsole("Connect CID=%s\r\n",cid);
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
        toConsole("toPachube() NOT CONNECTED! (%d) Exiting\r\n", status);
        discardResponse();
        return(0);
    }
	snprintf(databuffer, 36, "%d,%s",key,value);
	toRadio("\033S%dPUT /v2/feeds/%s.csv HTTP/1.1\r\n",atoi(cid),pachubeFeed);
	toRadio("User-Agent: SuspectDevices/Baco-matic5000 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.15\r\n");
	toRadio("Host: api.pachube.com\r\nAccept: */*\r\nConnection: close\r\n");
	toRadio("X-PachubeApiKey: %s\r\n",pachubeKey);
	toRadio("Content-Length: %d\r\n\r\n%s\r\n\033E",strlen(databuffer),databuffer);
	//toConsole("Sending to feed %s: (%d)%s \r\n",pachubeFeed,strlen(databuffer),databuffer);
    
    //
    // sent data, wait for an OK
    //
	while (!gotRadioLine()); 
	while (!(retval=isErrorOrOK(radioInBuffer))){
		//toConsole("%s\r\n",radioInBuffer);
		while (!gotRadioLine()) {
			;	
		}		
	}
    
	if (retval<1) {
        toConsole("NOT OK! %s\r\n",radioInBuffer);
        discardResponse();
        return(0);
    }
    //
    // should get an escaped stream followed by a disconnect
    //
	index=0;
    gotResponse=false;
	digitalWrite(ORN_LED,LOW);
    
    while (!gotRadioLine());
    sprintf(databuffer,"%c%c%c%c%cS%dHTTP",0x1b,0x4f,0x1b,0x4f,0x1b,atoi(cid)); // the double escapeing I dont get.
    if(!strncmp(databuffer, radioInBuffer,strlen(databuffer)) ) {
        databuffer[0]=radioInBuffer[16];
        databuffer[1]=radioInBuffer[17];
        databuffer[2]=radioInBuffer[18];
        databuffer[3]='\0';
        //toConsole("[%d] %s\r\n",retval=atoi(databuffer),radioInBuffer+7);
    }
    while (!gotRadioLine()); 
    discardResponse();
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
    
	discardResponse();
	toRadio("\r\n");
	discardResponse();
	toRadio("\r\nAT+NCTCP=216.52.233.122,80\r\n");
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
                    toConsole("Connect CID=%s\r\n",cid);
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
        toConsole("toPachube NOT CONNECTED! (%d) Exiting\r\n", status);
        discardResponse();
        return(0);
    }
	//snprintf(databuffer, 36, "%d,%s",key,value);
	toRadio("\033S%dHEAD / HTTP/1.1\r\n",atoi(cid),pachubeFeed);
	toRadio("User-Agent: SuspectDevices/Baco-matic5000 OpenSSL/0.9.8g zlib/1.2.3.3 libidn/1.15\r\n");
	toRadio("Host: api.pachube.com\r\nAccept: */*\r\nConnection: close\r\n");
	toRadio("X-PachubeApiKey: %s\r\n",pachubeKey);
	toRadio("Content-Length: 0\r\n\r\n\r\n\033E",strlen(databuffer),databuffer);
	//toConsole("Sending Head / request\r\n");
    
    //
    // sent data then wait for an OK
    //
	while (!gotRadioLine()); 
	while (!(retval=isErrorOrOK(radioInBuffer))){
		//toConsole("%s\r\n",radioInBuffer);
		while (!gotRadioLine()) {
			;	
		}		
	}
    
	if (retval<1) {
        toConsole("NOT OK! %s\r\n",radioInBuffer);
        discardResponse();
        return(0);
    }
    //
    // should get an escaped stream followed by a disconnect
    //
	index=0;
    gotResponse=false;
	digitalWrite(ORN_LED,LOW);
    
    while (!gotRadioLine());
    sprintf(databuffer,"%c%c%c%c%cS%dHTTP",0x1b,0x4f,0x1b,0x4f,0x1b,atoi(cid));
    if(!strncmp(databuffer, radioInBuffer,strlen(databuffer)) ) {
        databuffer[0]=radioInBuffer[16];
        databuffer[1]=radioInBuffer[17];
        databuffer[2]=radioInBuffer[18];
        databuffer[3]='\0';
        toConsole("[%d] %s\r\n",retval=atoi(databuffer),radioInBuffer+7);
    }
    
    while (!gotRadioLine()); 
    if(!strncmp("Date:",radioInBuffer,5)) {
        //Date: Sun, 03 Jun 2012 01:22:34 GMT
        //AT+SETTIME=<dd/mm/yyyy>,<HH:MM:SS>
        parseDate(radioInBuffer,databuffer); 
        retval=setRadioDate(databuffer);
    }
    digitalWrite(ORN_LED,HIGH);
    
    return(retval);
}

bool setRadioDate(char * datebuffer) { 
    bool retval;
    toRadio("AT+SETTIME=%s\r\n",datebuffer);
    while (!(retval=isErrorOrOK(radioInBuffer))){
        
        while (!gotRadioLine()) {
            ;	
        }		
    }
    delay(100);
    radioDate();
    return retval;
}
/*-----------------------------------------------------------------networkJoin()
 * join a network...
 *----------------------------------------------------------------------------*/

int networkJoin(char *ssid) {
    
    int retval=0;
    if (!hasWifiModule) return retval;
    strcpy(networkSSID, ssid);
    toConsole("joining: %s\r\n",networkSSID);
    Watchdog_Reset();
	discardResponse();
	toRadio("\r\nATE0\r\nAT+WA=%s\r\n",ssid);
	while (!gotRadioLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radioInBuffer))){
		//console.puts(radioInBuffer);
		while (!gotRadioLine()) { // timeout?
			;	
		}		
	}
    Watchdog_Reset();
	toRadio("\r\nAT+ndhcp=1\r\n");
    delay(1000); // this should be an idle.
	while (!gotRadioLine()); // and some timeout.
	while (!(isErrorOrOK(radioInBuffer))){
		toConsole("%s",radioInBuffer);
		while (!gotRadioLine()) {
			;	
		}		
	}
    //toConsole("join %s (%d)\r\n",networkSSID,retval=networkStatus());
	return(networkStatus());	
}

/*----------------------------------------------------------networkSetPassword()
 * set network password.
 *----------------------------------------------------------------------------*/

int networkSetPassword(char *pwd) {
    int retval=0;	
    //    strncpy(networkPassword, pwd, MAX_NETWORK_PASSWORD_LENGTH-1);
    strcpy(networkPassword, pwd);
	discardResponse();
	toRadio("\r\nAT+WWPA=%s\r\n",pwd);
	while (!gotRadioLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radioInBuffer))){
		//console.puts(radioInBuffer);
		while (!gotRadioLine()) {
			;	
		}
		
	}
    //toConsole("Password should be: %s (%d)\r\n",networkPassword,retval);
	return(retval>=1);
	
}
/*---------------------------------------------------------------networkStatus()
 * get network status.
 *----------------------------------------------------------------------------*/

int networkStatus() {
    int retval=0;
    if (!hasWifiModule) return retval;
    discardResponse();
	toRadio("\r\n");
	discardResponse();
	toRadio("\r\nAT+WSTATUS\r\n");
	while (!gotRadioLine()); // and some timeout.
	while (!(retval=isErrorOrOK(radioInBuffer))){
        if ((radioInBuffer[0]=='N') && (radioInBuffer[1] == 'O') && (radioInBuffer[2] == 'T')) {
            //if (strncmp("NOT ASSOCIATED",radioInBuffer,10) ==0){ //wtf????
            //toConsole("NOT ASSOCIATED\r\n");
            return(0);
        } else {
            //            toConsole("%s",radioInBuffer);
        }
		while (!gotRadioLine()) {
			// and some timeout!!!
            ;	
		}
	}
	//console.puts(radioInBuffer);
	return(retval>=1);
	
}
/*------------------------------------------------------------setupRadioModule()
 * 
 *----------------------------------------------------------------------------*/
void setupRadioModule(void) {
    long int mark=millis();
    toConsole("DBG: >>SetupRadioModule()");
	toRadio("\r\n");
	discardResponse();
    //toRadio("ATE0\r\nAT&K0\r\nAT&R0\r\nAT+WD\r\nAT+NCLOSEALL\r\n");
    toRadio("ATE0\r\nATH\r\n");
	while (!gotRadioLine());
	while ((millis()-mark<1000)&&(!(hasWifiModule=isErrorOrOK(radioInBuffer)))){
        toConsole("%s",radioInBuffer);
		while ((millis()-mark<1000)&&!gotRadioLine()) {
			;
		}
	}
    discardResponse();
    registerAction(_NSC_, &NSCaction);

    toConsole("DBG: <<SetupRadioModule()");
    
}


