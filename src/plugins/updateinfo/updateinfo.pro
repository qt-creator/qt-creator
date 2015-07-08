QT += network xml

HEADERS += updateinfoplugin.h \
    settingspage.h
SOURCES += updateinfoplugin.cpp \
    settingspage.cpp
FORMS += settingspage.ui
RESOURCES += updateinfo.qrc

isEmpty(UPDATEINFO_ENABLE):UPDATEINFO_EXPERIMENTAL_STR="true"
else:UPDATEINFO_EXPERIMENTAL_STR="false"

include(../../qtcreatorplugin.pri)
