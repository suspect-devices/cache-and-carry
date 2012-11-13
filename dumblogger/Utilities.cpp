/*------------------------------------------------------------------Utilities.cpp
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

#include "Utilities.h"

/*-----------------------------------------------------------------------------
 * file handling
 *----------------------------------------------------------------------------*/


/*----------------------------------------------------------------openNextFile() 
 * find next unopened file number.
 *----------------------------------------------------------------------------*/

int openNextFile() {
    dir_t p;//=&root;
    int current;
    int max=0;
    char numpart[5];
    root.rewind();
    while (root.readDir(&p)>0) {
        //Watchdog_Reset();
        togglePin(YEL_LED);
        togglePin(GRN_LED);
        
#ifdef REMOVE_ALL_ZERO_LENGTH_FILES
        // delete 0 length files;
        if (p.fileSize==0) {
            p.name[0] = DIR_NAME_DELETED;
            continue;
            
        }
#endif        
        if (!strncmp("LOG", p.name, 3) && !strncmp("TXT", p.name+8, 3)) {
            numpart[0]=p.name[3];    
            numpart[1]=p.name[4];    
            numpart[2]=p.name[5];
            numpart[3]=p.name[6];
            numpart[4]='\0';
            current=atoi(numpart);
            if ( current > max ) max=current;
        }
        /*toConsole("DBG: %c%c%c%c%c%c%c%c.%c%c%c (%d)\r\n", 
         p.name[0],p.name[1],p.name[2],p.name[3],
         p.name[4],p.name[5],p.name[6],p.name[7],
         p.name[8],p.name[9],p.name[10],max);
         */      
    }
    //SdVolume::CacheFlush(); to finish deleting files.
    return max;
    
}

char * nextFileName(bool rewind){
    static dir_t p;
    //bool done=false;
    if (rewind) root.rewind();
    if (root.readDir(&p)>0) {
        return p.name;
    }        
    return NULL;
}

uint8_t DIRaction(uint8_t source){
    bool firstname=true;
    char * filename;
    
    if (source==CONSOLE) {
        toConsole("SOD:");
        while ((filename=nextFileName(firstname))!=NULL) {
            toConsole("%s",filename);
            firstname=false;
        }
        toConsole("EOD:");
        return COMMAND_OK;
    } else {
        toDevice("NAK:");
        return COMMAND_IGNORED;
    }
    
    
}

uint8_t TYPaction(uint8_t source){
    char * filename=(char *)consoleComm.args;
    SdFile myFile;
    char buf[65];
    int n;
    if (source==CONSOLE) {
        toConsole("SOD:%s",filename);
        if( myFile.open(&root, filename, O_READ) ) {
            while ((n = myFile.read(buf, sizeof(buf)-1)) > 0) {
                buf[n]='\0';
                toConsole(buf);
            }
        }
        toConsole("EOD:%s",filename);
        return COMMAND_OK;
    } else {
        toDevice("NAK:");
        return COMMAND_IGNORED;
    }
    
    
}

int sdCardInit(void) {
    if (!card.init()) { 
        return(_SD_CARD_INIT_FAILED_);
    }
    delay(100);
    
    // initialize a FAT volume
    if (!volume.init(&card)) {
        return(_SD_VOLUME_INIT_FAILED_);
    }
    
    // open the root directory
    if (!root.openRoot(&volume)) {
        return(_SD_OPEN_ROOT_FAILED_);
    }
    return(_SD_CARD_OK_);
}



