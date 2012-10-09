#Boa:Frame:Frame1

import wx

def create(parent):
    return Frame1(parent)

[wxID_FRAME1, wxID_FRAME1CMD, wxID_FRAME1OUTPUT, wxID_FRAME1REBUILD, 
 wxID_FRAME1RESET, wxID_FRAME1STATUSBAR1, 
] = [wx.NewId() for _init_ctrls in range(6)]

class Frame1(wx.Frame):
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
        wx.Frame.__init__(self, id=wxID_FRAME1, name='', parent=prnt,
              pos=wx.Point(265, 195), size=wx.Size(838, 526),
              style=wx.DEFAULT_FRAME_STYLE, title='BOM5k Console')
        self.SetClientSize(wx.Size(838, 504))

        self.CMD = wx.TextCtrl(id=wxID_FRAME1CMD, name=u'CMD', parent=self,
              pos=wx.Point(0, 62), size=wx.Size(100, 22), style=0, value=u'>')
        self.CMD.Bind(wx.EVT_TEXT, self.OnCMDText, id=wxID_FRAME1CMD)
        self.CMD.Bind(wx.EVT_KILL_FOCUS, self.OnCMDKillFocus)

        self.OUTPUT = wx.TextCtrl(id=wxID_FRAME1OUTPUT, name=u'OUTPUT',
              parent=self, pos=wx.Point(0, 40), size=wx.Size(100, 22), style=0,
              value=u'...')

        self.REBUILD = wx.Button(id=wxID_FRAME1REBUILD, label=u'REBUILD',
              name=u'REBUILD', parent=self, pos=wx.Point(0, 20),
              size=wx.Size(84, 20), style=0)
        self.REBUILD.Bind(wx.EVT_BUTTON, self.OnREBUILDButton,
              id=wxID_FRAME1REBUILD)

        self.RESET = wx.Button(id=wxID_FRAME1RESET, label=u'RESET',
              name=u'RESET', parent=self, pos=wx.Point(0, 0), size=wx.Size(84,
              20), style=0)

        self.statusBar1 = wx.StatusBar(id=wxID_FRAME1STATUSBAR1,
              name='statusBar1', parent=self, style=0)

        self._init_sizers()

    def __init__(self, parent):
        self._init_ctrls(parent)

    def OnREBUILDButton(self, event):
        event.Skip()

    def OnCMDText(self, event):
        event.Skip()

    def OnCMDKillFocus(self, event):
        event.Skip()
