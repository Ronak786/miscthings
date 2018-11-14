import json
import os
import json
import socket

#from PyQt5.QtCore import QTime, QDate, QTimer, QDateTime
from PyQt5.QtCore import QTimer, QDateTime
#from PyQt5.QtWidgets import *

#for tcp
from PyQt5.QtNetwork import QHostAddress, QTcpServer
from PyQt5.QtNetwork import  QNetworkInterface, QAbstractSocket

from modules.base.deviceconfig import DeviceConfig
from modules.base.xwidget import XWidget
#from modules.device.transact.fimeintegrihelper import FimeIntegriHelper
from .ui_testtransact import Ui_Form

EMV_CONFIG_PATH_NAME = 'config'


class TestTransact(XWidget, Ui_Form):
    def __init__(self):
        super(TestTransact, self).__init__()
        self.setupUi(self)

        try:
            self._init_device_config()

            self._init_terminal_operation()
            self._init_online_parameters()
        except Exception as e:
            print("here exception")
            self.exception(e)

    # ********************************************** Init ****************************************************

    def _init_device_config(self):
        self.deviceconfig = DeviceConfig()
        self.deviceconfig.print_signal.connect(self.print_signal)
        self.layout_DeviceConfig.addWidget(self.deviceconfig)

    def _init_terminal_operation(self):
        self.port = 12345
        self.serveropened = False
        self._onPushButton_Openserver_Clicked()

    def _init_online_parameters(self):
        self.comboBox_AuthorizationResponseCode.addItems(['00:Approve', '51:Decline'])

        self._issuerdata = ['']
        try:
            with open('script/transact/issuerdata', 'r') as fd:
                for line in fd.readlines():
                    line = line.replace('\n', '').replace('\r', '')
                    self._issuerdata.append(line)
                fd.close()
        except Exception as e:
            self.exception(e)
        self.comboBox_IssuerAppData.addItems(self._issuerdata)

    def showEvent(self, QShowEvent):
        try:
            if self.config.get('comboBox_AuthorizationResponseCode'):
                self.comboBox_AuthorizationResponseCode.setCurrentText(self.config['comboBox_AuthorizationResponseCode'])
            if self.config.get('comboBox_IssuerAppData'):
                self.comboBox_IssuerAppData.setCurrentText(self.config['comboBox_IssuerAppData'])
        except Exception as e:
            self.exception(e)
        super().showEvent(QShowEvent)

    def hideEvent(self, QHideEvent):
        try:
            self.config['comboBox_AuthorizationResponseCode'] = self.comboBox_AuthorizationResponseCode.currentText()
            self.config['comboBox_IssuerAppData'] = self.comboBox_IssuerAppData.currentText()
        except Exception as e:
            self.exception(e)
        super().hideEvent(QHideEvent)

    # ********************************************** Slot ****************************************************

    def _onPushButton_Openserver_Clicked(self):
        print("start socker at ",self.port)

        if self.serveropened is False:
            #start init qtcpserver
            self.tcpServer = QTcpServer(self)
            address = QHostAddress('0.0.0.0')
            if not self.tcpServer.listen(address, self.port):
                print("cant listen!")
                self.close()
                return
            self.tcpServer.newConnection.connect(self.dealCommunication)
            self.fillActivateIps()

    def dealCommunication(self):
        # Get a QTcpSocket from the QTcpServer

        print("start handle")
        self.print_signal.emit("info", "connected")

        self.tcpSocket =  self.tcpServer.nextPendingConnection()
        #first read
        self.tcpSocket.readyRead.connect(self.showMessage)
        self.tcpSocket.disconnected.connect(self.showDisconnect)

    def showMessage(self):
        print ("start read show message")
        self.tcpSocket.readAll()

        #construct return data here
        origstr = self.comboBox_IssuerAppData.currentText()
        lenofstr = int(len(origstr) / 2)
        origstr2 = self.comboBox_AuthorizationResponseCode.currentText()
        finalstr = "8A02{:02}{:02}91{:02X}{}".format(int(origstr2[0]) + 30, int(origstr2[1])+30, lenofstr, origstr)
        print("return:", finalstr)

        self.tcpSocket.write(finalstr.encode("utf-8"))

    def showDisconnect(self):
        self.print_signal.emit("info", "disconnected")

    def fillActivateIps(self):
        print("start fill")
        for interface in QNetworkInterface.allInterfaces():
            iflags = interface.flags()
            if (iflags & QNetworkInterface.IsRunning) and  not (iflags  & QNetworkInterface.IsLoopBack) :
                for addrentry in interface.addressEntries():
#                    print("append ",addrentry.ip().toString(),addrentry.ip().protocol())
                    if addrentry.ip().protocol() == QAbstractSocket.IPv4Protocol:
                        self.plainTextEdit_iplist.appendPlainText("{}:{}".format(addrentry.ip().toString(), self.port))


