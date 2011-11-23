INCLUDEPATH *= $$PWD

QT += network
win32 {
    greaterThan(QT_MAJOR_VERSION, 4) {
        QT += core-private
    } else {
        include(../../private_headers.pri)
    }
}

# Input
HEADERS += $$PWD/symbianutils_global.h \
    $$PWD/callback.h \
    $$PWD/codautils.h \
    $$PWD/codautils_p.h \
    $$PWD/symbiandevicemanager.h \
    $$PWD/codadevice.h \
    $$PWD/codamessage.h \
    $$PWD/virtualserialdevice.h

SOURCES += $$PWD/codautils.cpp \
    $$PWD/symbiandevicemanager.cpp \
    $$PWD/codadevice.cpp \
    $$PWD/codamessage.cpp \
    $$PWD/virtualserialdevice.cpp

DEFINES += HAS_SERIALPORT
win32:SOURCES += $$PWD/virtualserialdevice_win.cpp
unix:SOURCES += $$PWD/virtualserialdevice_posix.cpp

macx:LIBS += -framework IOKit -framework CoreFoundation
include(../../shared/json/json.pri)

DEFINES += JSON_INCLUDE_PRI

contains(CONFIG, dll) {
    DEFINES += SYMBIANUTILS_BUILD_LIB
} else {
    DEFINES += SYMBIANUTILS_BUILD_STATIC_LIB
}

