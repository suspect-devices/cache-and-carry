#!/usr/bin/python
"""
*-----------------------------------------------------------------------AMAv2.py
* XtractWork
*
* Created by Donald D Davis on 9/11/12.
* Copyright 2012 Suspect Device, for xtractSolutions . All rights reserved.
*
* Python Class to simplify interaction between the monitor on the AMA and 
* programs to monitor and control it, the class uses a dispatch table to 
* define the actions. The open command can be used to identify both the ama 
* port and the logger and to identify an ama that is attached through the logger.
*
* the class/behaviour model should be used to create the next gen of bom code.
*
* you can use this to identify the logger or you can superclass the AMAv2
* to create at BOM5k class. (I dont know which is more appropriate yet)
*
* The test case for the class below demonstrates how to attach the a 
* function to the dispatch table using the log keyword to push data to 
* the google app.  
*
*
"""

import re
import string
import datetime
import serial
import os
import os.path
import shutil
import platform
import sys
import time
import re

def list_serial_ports():
  def full_port_name(portname):
    """ Given a port-name (of the form COM7, COM12, CNCA0, etc.) returns a full
        name suitable for opening with the Serial class.
        http://eli.thegreenplace.net/2009/07/31/listing-all-serial-ports-on-windows-with-python/
    """
    m = re.match('^COM(\d+)$', portname)
    if m and int(m.group(1)) < 10:
        return portname
    return '\\\\.\\' + portname

  plat_sys = platform.system()
  plat_bits = platform.architecture()[0]

  if plat_sys == 'Windows':
    import _winreg as reg
    p = 'HARDWARE\\DEVICEMAP\\SERIALCOMM'
    k = reg.OpenKey(reg.HKEY_LOCAL_MACHINE, p)
    possible_paths = []
    i = 0
    while True:
        try:
            possible_paths.append(full_port_name(reg.EnumValue(k, i)[1]))
            i += 1
        except WindowsError:
            break
  else:
    if plat_sys == 'Linux':
       file_prefix='ttyACM'
    elif plat_sys == 'Darwin':
       file_prefix='cu.usbmodem'
    
    possible_paths = [os.path.join('/dev', x) for x in os.listdir('/dev') \
                          if x.startswith(file_prefix)]
  if len(possible_paths) == 0:
    return None
  return possible_paths


#from SDserialutilities import list_serial_ports

class AMAv2:

    def __init__(self, portname=None, baudrate=9600, logger_only=False):
        self.ser=None
        self.attached=False
        self.lastcontact=None
        self.lastTimestamp=None
        self.command_state=-1
        self.acknowleged=False
        self.dispatch_table={
            'SSN':self.ssn,'HWV':self.hwv,'SWV':self.swv,'LOG':self.log,
            'LSV':self.lsv,'ACK':self.ack,'DVP':self.dvp,'CMD':self.cmd,
            'DBG':self.dbg,'ALT':self.alt,'WRN':self.alt,'STC':self.alt
        }
        self.status={
            'TS1':None,'TS2':None,'TS3':None,'TS4':None,'TS5':None,
            'FAP':None,'FAN':None,'CHL':None,'PWR':None,'THR':None,
            'EC1':None,'EC2':None,'EC2':None
        }
        
        for var in self.status.keys():
           self.dispatch_table[var]=self.update_state_variable

        #for var in ['FAN','CHL']:
        #    self.dispatch_table[var]=self.update_state_integer
        #for var in ['TS1','TS2','TS3','TS4','TS5']:
        #    self.dispatch_table[var]=self.update_state_float
        
        self.open(portname,baudrate,logger_only)
        

    def yank_timestamp(self,value):
        if value is None:
            return None
        try:
            
            ll=value.split(',',2)
            #print value,"->",ll,len(ll)
            if len(ll) == 1:
                self.lastcontact=datetime.datetime.utcnow()
                return ll[0]
            else:
                self.lastcontact=datetime.datetime.utcnow()
                self.lastTimestamp=ll[0]
                return ll[1]
        except Exception as e:
            print "error in yank timestamp:", str(e)
            return None
            
    def update_state_variable(self,key,verb,value):
        self.status[key]=self.yank_timestamp(value)
        #should maybe flag the update somewhere
        #may want ot return something.
        
    def set_dispatch_function(self,key,d_fun):
        #should check key value and callable here....
        self_dispatch_table[key]=d_fun
        
    def log(self,key='LOG',verb=':',value=None):
        if value is not None:
             print "LOG:"+value.rstrip('\r\n') 
        return value

 
    def alt(self,key='ALT',verb='?',value=None):
        if value is not None:
            print key+":"+value.rstrip('\r\n')
        return value

    def ssn(self,key='SSN',verb='?',value=None):
        if (value is not None):
            ssn=self.yank_timestamp(value).rstrip('\r\n')
            if ( len(ssn)>9  and all(char in string.hexdigits for char in ssn) ):
                self.device_ssn_string=ssn
            else:
                self.sendCMD("SSN?") #we got garbage ask again
        return self.device_ssn_string

    def hwv(self,key='HWV',verb='?',value=None):
        if value is not None:
            self.device_hwv_string=self.yank_timestamp(value)
        return self.device_hwv_string

    def swv(self,key='SWV',verb='?',value=None):
        if value is not None:
            self.device_swv_string=self.yank_timestamp(value)
        return self.device_swv_string

    def lsv(self,key='LSV',verb=':',value=None):
        if value is not None:
            print "LSV",
            self.logger_swv_string=self.yank_timestamp(value)
            print self.logger_swv_string
        return self.logger_swv_string

    def ack(self,key='ACK',verb='?',value=None):
        if value is not None:
            self.yank_timestamp(value)
            self.acknowleged=True
        return self.acknowleged

    def cmd(self,key='CMD',verb='?',value=None):
        if value is not None:
            self.command_state= "ON" in self.yank_timestamp(value)
        return self.command_state

    def dvp(self,key='DVP',verb='?',value=None):
        if value is not None:
            self.device_on_logger = not "NOT" in self.yank_timestamp(value)
        return self.device_on_logger

    def dbg(self,key='DBG',verb=':',value=None):
        if value is not None:
            print key+":"+value
        #return something probably
        
    def sendCMD(self,cmd=None):
        if cmd is not None and self.ser is not None:
            self.ser.write(cmd+'\r\n')
        #return something probably
        
    def nop(self,key='NOP',verb=':',value=None):
        print key,"DBG:("+key+")not implimented!!!"


    def open(self,portname,baudrate,logger_only=False,printDebug=True):
        attempts=2
        line=''
        self.version_string=None
        self.device_ssn_string=None
        self.device_swv_string=None
        self.device_dbg_level=None
        self.device_log_level=None
        self.logger_swv_string=None
        self.device_on_logger=False
        self.portname=None
        self.logger_portname=None
        self.lastconact=None
        self.initialContact=None
        self.ser=None
        while attempts and self.portname is None:
          ports=list_serial_ports()
          if ports is None:
            break
          else:
            for port in ports:
                try:
                    self.initialContact=[]
                    ser=serial.Serial(port,9600,timeout=1)#,dsrdtr=True,rtscts=True)
                    if not ser.isOpen:
                        continue
                    ser.flushInput()
                    time.sleep(.1)
                    try_no=1
                    while not ser.inWaiting():
                        time.sleep(.15)
                        try_no=try_no+1
                        if try_no > 15:
                            break
                            
                    try_no=1
                    ser.write('SYN!\r\nLSV?\r\nDVP?\r\nDLG!0\r\nSSN?\r\nSWV?\r\nHWV?\r\n')
                    while not ser.inWaiting():
                        time.sleep(.15)
                        try_no=try_no+1
                        if try_no > 15:
                            break
                    
                    while ser.inWaiting():                            
                        line=unicode(ser.readline(100).rstrip('\r\n'), errors='ignore')
                        line=line.encode('ascii','ignore')
                        if len(line.split('\r'))>1:
                            line=line.split('\r')[1] # if input echo is on strip input.
                        #print '\r\n'+repr(line)+'\r\n'
                        if len(line) > 3 and line[3] == ':' :
                            self.dispatch(line)
                        else:
                           print "DBG:GARBAGE >>"+line+"<<"
                           #check stream for rev 1 hardware
                        self.initialContact.append(line)
                        #time.sleep(.1)
                        if not logger_only:
                            time.sleep(.35)
                    
                except Exception as e:
                    print line
                    print "Error in open :", str(e)
                    
                if self.logger_swv_string is not None:
                    print "DBG:Found a Logger : version =", self.logger_swv_string
                    self.logger_portname=port 
                    if not self.device_on_logger:
                        self.device_swv_string=None
                    if logger_only:
                        self.portname=port
                        self.ser=ser

                if self.device_swv_string is not None:
                    print "DBG:Found an AMA : version =", self.device_swv_string
                    print "DBG:SSN =", self.device_ssn_string
                    self.portname=port
                    self.ser=ser
                    break
                if self.portname is None:
                    ser.close()
                attempts=attempts-1
        self.attached = (self.ser is not None)
        print ":", self.attached, self.logger_swv_string,self.ser
    
    def dispatch(self,buffer):
        if buffer is None:
            print "DBG:short read"
            return
        if (len(buffer) < 4) or buffer=='':
            print "DBG:"+repr(buffer.rstrip('\r\n'))
            return
        key=buffer[0:3]
        verb=buffer[3]
        if len(buffer) > 4:
            value=buffer[4:].rstrip('\r\n')
        else:
            value=''
        self.dispatch_table.get(key,self.nop)(key,verb,value)
"""
*----------------------------------------------------------------------test code
* 
*
""" 
 
    
if __name__ == "__main__":
    
    
    import httplib, urllib
    
    def log_to_app(key,verb=':',value=None):
        if value is not None and device.device_ssn_string is not None : 
            try:
                print "LOG:(" + device.device_ssn_string + ")" + value
                ssn=device.device_ssn_string
                (timestamp,ambient,chiller1,chiller2,air1,air2,chiller,
                    fan,state) = value.split(',')
                #print "SANITY CHECK", ssn, timestamp, ambient
                params = urllib.urlencode({#'ssn': ssn,
                                            'timestamp':timestamp,
                                            'ts1': ambient,
                                            'ts2': chiller1,'ts3': chiller2,
                                            'ts4': air1, 'ts5': air2,
                                            'chiller': chiller, 
                                            'fan': fan, 
                                            'state': state 
                                            
                                            })
                headers = {"Content-type": "application/x-www-form-urlencoded",
                           "Accept": "text/plain"}
                conn = httplib.HTTPConnection("patch-bay.appspot.com")
            #    need to deal with bogus ssn value
                conn.request("POST", "/log?ssn="+ssn, params, headers)
                response = conn.getresponse()
                print "DBG:"+str(response.status),response.reason,
                data = response.read()
                print data.rstrip('\r\n')
                conn.close()
            except Exception as e:
                print "DBG:NETWORK?"+str(e)
    
    def test_stc (key,verb,value):
        print key,verb,value,device.device_ssn_string

       
    device=AMAv2(logger_only=True)
    if device.ser is None:
        print "FOUND NO AMA DEVICE"
        exit()
    device.dispatch_table['LOG']=log_to_app
    while True:
        if device.ser.inWaiting():
            line=device.ser.readline(100).rstrip('\r\n')
            line=line.encode('ascii', 'ignore')
                    
            device.dispatch(line)
    
