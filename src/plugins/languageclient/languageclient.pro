include(../../qtcreatorplugin.pri)

DEFINES += LANGUAGECLIENT_LIBRARY

HEADERS += \
    client.h \
    dynamiccapabilities.h \
    languageclient_global.h \
    languageclientcompletionassist.h \
    languageclientinterface.h \
    languageclientmanager.h \
    languageclientoutline.h \
    languageclientplugin.h \
    languageclientquickfix.h \
    languageclientsettings.h \
    languageclientutils.h


SOURCES += \
    client.cpp \
    dynamiccapabilities.cpp \
    languageclientcompletionassist.cpp \
    languageclientinterface.cpp \
    languageclientmanager.cpp \
    languageclientoutline.cpp \
    languageclientplugin.cpp \
    languageclientquickfix.cpp \
    languageclientsettings.cpp \
    languageclientutils.cpp

RESOURCES += \
    languageclient.qrc
