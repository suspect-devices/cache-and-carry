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
//find next unopened file.
int openNextFile() {
    dir_t p;//=&root;
    int current;
    int max=0;
    char numpart[4];
    root.rewind();
    while (root.readDir(p)>0) {
        //Watchdog_Reset();
        togglePin(YEL_LED);
        togglePin(GRN_LED);
        
        // done if past last used entry
        if (p.name[0] == DIR_NAME_FREE) break;
        
        // skip deleted entry and entries for . and  ..
        if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;
        
        // only list subdirectories and files
        if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;
        
#ifdef REMOVE_ALL_ZERO_LENGTH_FILES
        // delete 0 length files;
        if (p.fileSize==0) {
            p.name[0] = DIR_NAME_DELETED;
            continue;
            //SdVolume::CacheFlush();
        }
#endif        
        if (!strncmp("LOG", p.name, 3) && !strncmp("TXT", p.name+8, 3)) {
            numpart[0]=p.name[3];    
            numpart[1]=p.name[4];    
            numpart[2]=p.name[5];    
            numpart[3]='\0';
            current=atoi(numpart);
            if ( current > max ) max=current;
        }
        /*console.printf("DBG: %c%c%c%c%c%c%c%c.%c%c%c (%d)\r\n", 
         p.name[0],p.name[1],p.name[2],p.name[3],
         p.name[4],p.name[5],p.name[6],p.name[7],
         p.name[8],p.name[9],p.name[10],max);
         */      
    }
    return max;
    
}

