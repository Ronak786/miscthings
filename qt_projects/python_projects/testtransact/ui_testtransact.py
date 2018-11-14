# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '.\modules\device\testtransact\ui_testtransact.ui'
#
# Created by: PyQt5 UI code generator 5.9
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_Form(object):
    def setupUi(self, Form):
        Form.setObjectName("Form")
        Form.resize(623, 526)
        self.horizontalLayout_2 = QtWidgets.QHBoxLayout(Form)
        self.horizontalLayout_2.setObjectName("horizontalLayout_2")
        self.verticalLayout_13 = QtWidgets.QVBoxLayout()
        self.verticalLayout_13.setSizeConstraint(QtWidgets.QLayout.SetFixedSize)
        self.verticalLayout_13.setObjectName("verticalLayout_13")
        self.layout_DeviceConfig = QtWidgets.QHBoxLayout()
        self.layout_DeviceConfig.setSizeConstraint(QtWidgets.QLayout.SetFixedSize)
        self.layout_DeviceConfig.setObjectName("layout_DeviceConfig")
        self.verticalLayout_13.addLayout(self.layout_DeviceConfig)
        self.groupBox_6 = QtWidgets.QGroupBox(Form)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.groupBox_6.sizePolicy().hasHeightForWidth())
        self.groupBox_6.setSizePolicy(sizePolicy)
        self.groupBox_6.setObjectName("groupBox_6")
        self.verticalLayout_3 = QtWidgets.QVBoxLayout(self.groupBox_6)
        self.verticalLayout_3.setObjectName("verticalLayout_3")
        self.formLayout = QtWidgets.QFormLayout()
        self.formLayout.setSpacing(10)
        self.formLayout.setObjectName("formLayout")
        self.checkBox_Response = QtWidgets.QCheckBox(self.groupBox_6)
        self.checkBox_Response.setChecked(True)
        self.checkBox_Response.setObjectName("checkBox_Response")
        self.formLayout.setWidget(0, QtWidgets.QFormLayout.LabelRole, self.checkBox_Response)
        self.label = QtWidgets.QLabel(self.groupBox_6)
        self.label.setEnabled(True)
        self.label.setMinimumSize(QtCore.QSize(0, 30))
        self.label.setWordWrap(True)
        self.label.setObjectName("label")
        self.formLayout.setWidget(1, QtWidgets.QFormLayout.LabelRole, self.label)
        self.comboBox_AuthorizationResponseCode = QtWidgets.QComboBox(self.groupBox_6)
        self.comboBox_AuthorizationResponseCode.setEditable(True)
        self.comboBox_AuthorizationResponseCode.setObjectName("comboBox_AuthorizationResponseCode")
        self.formLayout.setWidget(1, QtWidgets.QFormLayout.FieldRole, self.comboBox_AuthorizationResponseCode)
        self.label_2 = QtWidgets.QLabel(self.groupBox_6)
        self.label_2.setObjectName("label_2")
        self.formLayout.setWidget(2, QtWidgets.QFormLayout.LabelRole, self.label_2)
        self.comboBox_IssuerAppData = QtWidgets.QComboBox(self.groupBox_6)
        self.comboBox_IssuerAppData.setEditable(False)
        self.comboBox_IssuerAppData.setObjectName("comboBox_IssuerAppData")
        self.formLayout.setWidget(2, QtWidgets.QFormLayout.FieldRole, self.comboBox_IssuerAppData)
        self.verticalLayout_3.addLayout(self.formLayout)
        self.verticalLayout_13.addWidget(self.groupBox_6)
        self.label_iplist = QtWidgets.QLabel(Form)
        self.label_iplist.setObjectName("label_iplist")
        self.verticalLayout_13.addWidget(self.label_iplist)
        self.plainTextEdit_iplist = QtWidgets.QPlainTextEdit(Form)
        self.plainTextEdit_iplist.setReadOnly(True)
        self.plainTextEdit_iplist.setObjectName("plainTextEdit_iplist")
        self.verticalLayout_13.addWidget(self.plainTextEdit_iplist)
        spacerItem = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout_13.addItem(spacerItem)
        self.horizontalLayout_2.addLayout(self.verticalLayout_13)
        self.verticalLayout_2 = QtWidgets.QVBoxLayout()
        self.verticalLayout_2.setObjectName("verticalLayout_2")
        spacerItem1 = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout_2.addItem(spacerItem1)
        self.horizontalLayout_2.addLayout(self.verticalLayout_2)

        self.retranslateUi(Form)
        QtCore.QMetaObject.connectSlotsByName(Form)

    def retranslateUi(self, Form):
        _translate = QtCore.QCoreApplication.translate
        Form.setWindowTitle(_translate("Form", "Form"))
        self.groupBox_6.setTitle(_translate("Form", "Online Parameters"))
        self.checkBox_Response.setText(_translate("Form", "Response"))
        self.label.setText(_translate("Form", "Authorization    Response Code[8A]"))
        self.label_2.setText(_translate("Form", "Issuer AppData[91]"))
        self.label_iplist.setText(_translate("Form", "IP List"))

