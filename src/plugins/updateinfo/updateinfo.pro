TARGET = UpdateInfo
TEMPLATE = lib
QT += network xml

HEADERS += updateinfoplugin.h \
    updateinfobutton.h
SOURCES += updateinfoplugin.cpp \
    updateinfobutton.cpp
RESOURCES += updateinfo.qrc

isEmpty(UPDATEINFO_ENABLE):UPDATEINFO_EXPERIMENTAL_STR="true"
else:UPDATEINFO_EXPERIMENTAL_STR="false"

include(../../qtcreatorplugin.pri)
include(updateinfo_dependencies.pri)
