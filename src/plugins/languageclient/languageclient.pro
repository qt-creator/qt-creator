include(../../qtcreatorplugin.pri)

DEFINES += LANGUAGECLIENT_LIBRARY

HEADERS += \
    baseclient.h \
    dynamiccapabilities.h \
    languageclient_global.h \
    languageclientcodeassist.h \
    languageclientmanager.h \
    languageclientoutline.h \
    languageclientplugin.h \
    languageclientsettings.h \
    languageclientutils.h


SOURCES += \
    baseclient.cpp \
    dynamiccapabilities.cpp \
    languageclientcodeassist.cpp \
    languageclientmanager.cpp \
    languageclientoutline.cpp \
    languageclientplugin.cpp \
    languageclientsettings.cpp \
    languageclientutils.cpp

RESOURCES += \
    languageclient.qrc
