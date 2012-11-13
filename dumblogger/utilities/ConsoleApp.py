#!/usr/bin/env python

import wx
import subprocess
import datetime
import time
import os
from wx.lib.embeddedimage import PyEmbeddedImage
import sys
from AMAv2 import AMAv2
import icons

libmaple_directory=os.path.dirname(os.path.realpath(__file__)) + os.sep + '../../../../libmaple/'
program_code_directory='../programs/dumblogger'
my_dir=os.path.dirname(os.path.realpath(__file__))


[wxID_CONSOLE_FRAME, wxID_CONSOLE_FRAMECMD, wxID_CONSOLE_FRAMEOUTPUT, wxID_CONSOLE_FRAMEREBUILD, 
 wxID_CONSOLE_FRAMERESET, wxID_CONSOLE_FRAMESTATUSBAR1, 
] = [wx.NewId() for _init_ctrls in range(6)]

class ConsoleFrame(wx.Frame):
    def _init_coll_boxSizer1_Items(self, parent):
        # generated method, don't edit

        parent.AddSizer(self.boxSizer2, 0, border=0, flag=wx.EXPAND)
        parent.AddSizer(self.boxSizer3, 1, border=0, flag=wx.EXPAND)
        parent.AddWindow(self.statusBar1, 0, border=0, flag=0)

    def _init_coll_boxSizer2_Items(self,parent):
        parent.AddWindow(self.RESET, 0, border=0, flag=0)
        parent.AddWindow(self.REBUILD, 0, border=0, flag=0)

    def _init_coll_boxSizer3_Items(self, parent):
        parent.AddWindow(self.OUTPUT, 1, border=0, flag=wx.EXPAND)
        parent.AddWindow(self.CMD, 0, border=0, flag=wx.EXPAND)

    def _init_sizers(self):
        # generated method, don't edit
        self.boxSizer1 = wx.BoxSizer(orient=wx.VERTICAL)

        self.boxSizer2 = wx.BoxSizer(orient=wx.HORIZONTAL)

        self.boxSizer3 = wx.BoxSizer(orient=wx.VERTICAL)

        self._init_coll_boxSizer1_Items(self.boxSizer1)
        self._init_coll_boxSizer2_Items(self.boxSizer2)
        self._init_coll_boxSizer3_Items(self.boxSizer3)

        self.SetSizer(self.boxSizer1)

    def _init_ctrls(self, prnt):
        # generated method, don't edit
        wx.Frame.__init__(self, id=wxID_CONSOLE_FRAME, name='', parent=prnt,
              pos=wx.Point(265, 195), size=wx.Size(838, 526),
              style=wx.DEFAULT_FRAME_STYLE, title='BOM5k Console')
        self.SetClientSize(wx.Size(838, 504))

        self.CMD = wx.TextCtrl(id=wxID_CONSOLE_FRAMECMD, name=u'CMD', parent=self,
              pos=wx.Point(0, 62), size=wx.Size(100, 22), style=wx.TE_PROCESS_ENTER|wx.PROCESS_ENTER, value=u'')
        self.CMD.Bind(wx.EVT_TEXT_ENTER, self.OnCMDText, id=wxID_CONSOLE_FRAMECMD)
        self.CMD.Bind(wx.EVT_KILL_FOCUS, self.OnCMDKillFocus)

        self.OUTPUT = wx.TextCtrl(id=wxID_CONSOLE_FRAMEOUTPUT, name=u'OUTPUT',
              parent=self, pos=wx.Point(0, 40), size=wx.Size(100, 22), style=wx.TE_MULTILINE|wx.VSCROLL,
              value=u'')
        self.OUTPUT.SetEditable(False)
        #self.OUTPUT.SetAutoLayout(True)
        self.OUTPUT.Enable(False)

        self.REBUILD = wx.BitmapButton(id=wxID_CONSOLE_FRAMEREBUILD, #label=u'RESET',
              bitmap=icons.updateIcon.Image.ConvertToBitmap(),
              name=u'RESET', parent=self, pos=wx.Point(0, 0), #size=wx.Size(32,32), 
              style=0)
        self.REBUILD.Bind(wx.EVT_BUTTON, self.OnREBUILDButton,
              id=wxID_CONSOLE_FRAMEREBUILD)

        self.RESET = wx.BitmapButton(id=wxID_CONSOLE_FRAMERESET, #label=u'RESET',
              bitmap=icons.developerIcon.Image.ConvertToBitmap(),
              name=u'RESET', parent=self, pos=wx.Point(0, 0), #size=wx.Size(32,32),
               style=0)

        self.statusBar1 = wx.StatusBar(id=wxID_CONSOLE_FRAMESTATUSBAR1,
              name='statusBar1', parent=self, style=0)
        self.statusBar1.SetFieldsCount(4)
        self.statusBar1.SetStatusWidths([-1,-1,-1,-1])

        self._init_sizers()

    def __init__(self, parent):
        self._init_ctrls(parent)
        MenuBar = wx.MenuBar()
        print wx.StandardPaths.Get().GetResourcesDir()
        FileMenu = wx.Menu()
        
        item = FileMenu.Append(wx.ID_EXIT, "Quit\tctrl-Q","BAIL")
        self.Bind(wx.EVT_MENU, self.OnQuit, item)

        item = FileMenu.Append(wx.ID_ANY, 'Open Program File\tctrl-O', 'Reload Firmware')
        self.Bind(wx.EVT_MENU, self.OnOpen, item)

        item = FileMenu.Append(wx.ID_PREFERENCES, 'Preferences\tctrl-,','Set Preferences')
        self.Bind(wx.EVT_MENU, self.OnPrefs, item)

        MenuBar.Append(FileMenu, "&File")
        
        HelpMenu = wx.Menu()

        item = HelpMenu.Append(wx.ID_HELP, 'Help\tctrl-?',
                                "help at some point")
        self.Bind(wx.EVT_MENU, self.OnHelp, item)

        ## this gets put in the App menu on OS-X
        item = HelpMenu.Append(wx.ID_ABOUT, "&About",
                                "More information About this program")
        self.Bind(wx.EVT_MENU, self.OnAbout, item)
        MenuBar.Append(HelpMenu, "&Help")

        self.SetMenuBar(MenuBar)
        
        self.device_was_already_attached=False
        self.device=AMAv2(logger_only=True)
        self.serialTimer = wx.Timer(self,-1)
        self.Bind(wx.EVT_TIMER, self.processSerial, self.serialTimer)
        self.serialTimer.Start(750)
            
        self.libmaple_directory=libmaple_directory
        self.program_code_directory=program_code_directory


        
    def OnQuit(self,Event):
        self.Destroy()
        
    def OnAbout(self, event):
        dlg = wx.MessageDialog(self, "This is a small program to test\n"
                                     "the use of menus on Mac, etc.\n",
                                "About Me", wx.OK | wx.ICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()

    def OnHelp(self, event):
        dlg = wx.MessageDialog(self, "This would be help\n"
                                     "If there was any\n",
                                "Test Help", wx.OK | wx.ICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()

    def OnOpen(self, event):
        dialog = wx.DirDialog(self, "Program Directory :", style=wx.DD_DIR_MUST_EXIST )
        if dialog.ShowModal() == wx.ID_OK:
            #libmaple_directory = dialog.GetPath()
            self.program_code_directory=dialog.GetPath()
            self.libmaple_directory=self.program_code_directory + os.sep + '../../libmaple/'
            self.OnREBUILDButton(event)
        dialog.Destroy()

    def OnPrefs(self, event):
        dlg = wx.MessageDialog(self, "This would be an preferences Dialog\n"
                                     "If there were any preferences to set.\n",
                                "Preferences", wx.OK | wx.ICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()
        


    def OnREBUILDButton(self, event):
        self.statusBar1.SetStatusText("...Uploading...",1)
        self.OUTPUT.AppendText('\n---------------------- UPLOAD -------------------\n')
        if self.device.ser is not None:
            self.device.ser.close()
        self.device.attached=False
        try: 
            self.OUTPUT.AppendText(subprocess.check_output("make install",
                              env={'BOARD': 'maple_mini','USER_MODULES': self.program_code_directory,
                              'PATH': '/usr/local/arm-none-eabi/bin/:/usr/local/bin/:/usr/bin:/bin/:$PATH'},
                              #stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              shell=True,
                              cwd=self.libmaple_directory))
            self.OUTPUT.AppendText('\n------------------- SUCCESS ------------------\n')

        except subprocess.CalledProcessError as e:
            self.OUTPUT.AppendText(e.cmd +"Returned "+str(e.returncode))
            self.OUTPUT.AppendText(e.output)
            self.OUTPUT.AppendText('\n-------------------- FAIL --------------------\n')
        except Exception as e:
            self.OUTPUT.AppendText(str(e))
            self.OUTPUT.AppendText('\n-------------------- FAIL --------------------\n')
        

        time.sleep(1)
        self.OnReconnect()
        #self.device.attached=True # should be handled by ama class
        if event is not None:
            event.Skip()

    def OnCMDText(self, event):
        comm=self.CMD.GetValue().rstrip('\r\n')
        self.CMD.SetValue('')
        self.statusBar1.SetStatusText(">>"+comm,0)
        if self.device.attached:
            self.device.ser.write(str(comm)+'\r\n')
        event.Skip()

    def OnReconnect(self, event=None):
        self.device.attached=False
        if self.device.ser is not None:
            self.device.ser.close()
        self.statusBar1.SetStatusText("...Searching...",1)
        self.device.open(self.device.portname,9600,logger_only=True)
        #print "?",self.device.attached, self.device.logger_swv_string, self.device.ser
        if self.device.ser is not None and self.device.ser.isOpen():
            self.statusBar1.SetStatusText(self.device.logger_swv_string,1)
        if event is not None:
            event.Skip()


    def OnCMDKillFocus(self, event):
        event.Skip()
    
    
    def processSerial(self,event=None):
        if self.device.attached:
            if not self.device_was_already_attached: 
                self.statusBar1.SetStatusText(self.device.logger_swv_string,1)
            try:
                while self.device.ser.inWaiting():
                    line=unicode(self.device.ser.readline(100).rstrip('\r\n'),errors='ignore')
                    line=line.encode('ascii','replace')
                    self.OUTPUT.AppendText(line+'\n')
                    self.device.dispatch(line)
            except IOError as e:
                self.device.attached=False
        else:
            self.OnReconnect()
        self.device_was_already_attached=self.device.attached
        if event is not None:
            event.Skip()
            
# stubs for mac events...


class ConsoleApp(wx.App):
    def OnInit(self):
        self.main = ConsoleFrame(None)
        self.main.Show()
        self.SetTopWindow(self.main)
        print sys.argv
        if len(sys.argv)>1:
            self.main.program_code_directory = os.path.dirname(os.path.realpath(sys.argv[1]))
            self.main.libmaple_directory=self.main.program_code_directory + os.sep + '../../../libmaple/'
            self.main.OnREBUILDButton(None)
 
        return True

    def MacOpenFile(self, filename):
        """Called for files droped on dock icon, or opened via finders context menu"""
        print filename
        print "%s dropped on app"%(filename) #code to load filename goes here.
        self.main.program_code_directory = os.path.dirname(os.path.realpath(filename))+os.sep + '../'
        self.main.libmaple_directory=os.path.dirname(os.path.realpath(filename)) + os.sep + '../../../libmaple/'

        self.main.OnREBUILDButton(None)
        
    def MacReopenApp(self):
        """Called when the doc icon is clicked, and ???"""
        print "hey!"
        pass #self.main.BringWindowToFront()

    def MacNewFile(self):
        pass
    
    def MacPrintFile(self, file_path):
        pass
    


def main():
    application = ConsoleApp(0)
    application.MainLoop()

if __name__ == '__main__':
    main()
