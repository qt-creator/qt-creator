include(../../qtcreatorplugin.pri)

DEFINES += LANGUAGECLIENT_LIBRARY

HEADERS += \
    baseclient.h \
    dynamiccapabilities.h \
    languageclient_global.h \
    languageclientcodeassist.h \
    languageclientmanager.h \
    languageclientplugin.h \
    languageclientsettings.h

SOURCES += \
    baseclient.cpp \
    dynamiccapabilities.cpp \
    languageclientcodeassist.cpp \
    languageclientmanager.cpp \
    languageclientplugin.cpp \
    languageclientsettings.cpp

RESOURCES += \
    languageclient.qrc
