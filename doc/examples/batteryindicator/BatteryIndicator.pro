#-------------------------------------------------
#
# Project created by QtCreator 2010-05-26T16:46:58
#
#-------------------------------------------------

QT       += core gui

TARGET = BatteryIndicator
TEMPLATE = app


SOURCES += main.cpp\
        batteryindicator.cpp

HEADERS  += batteryindicator.h

FORMS    += batteryindicator.ui

CONFIG += mobility
MOBILITY = systeminfo

symbian {
    TARGET.UID3 = 0xecbd72d7
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}
