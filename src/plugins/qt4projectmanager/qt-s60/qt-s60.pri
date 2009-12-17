!isEmpty(SUPPORT_QT_S60) { 
    message("Adding experimental support for Qt/S60 applications.")
    DEFINES += QTCREATOR_WITH_S60
}
SOURCES += $$PWD/s60devices.cpp \
    $$PWD/s60devicespreferencepane.cpp \
    $$PWD/s60manager.cpp \
    $$PWD/winscwtoolchain.cpp \
    $$PWD/gccetoolchain.cpp \
    $$PWD/s60emulatorrunconfiguration.cpp \
    $$PWD/s60devicerunconfiguration.cpp \
    $$PWD/s60devicerunconfigurationwidget.cpp \
    $$PWD/serialdevicelister.cpp \
    $$PWD/rvcttoolchain.cpp \
    $$PWD/s60runconfigbluetoothstarter.cpp \
    $$PWD/abldparser.cpp \
    $$PWD/rvctparser.cpp \
    $$PWD/winscwparser.cpp
HEADERS += $$PWD/s60devices.h \
    $$PWD/s60devicespreferencepane.h \
    $$PWD/s60manager.h \
    $$PWD/winscwtoolchain.h \
    $$PWD/gccetoolchain.h \
    $$PWD/s60emulatorrunconfiguration.h \
    $$PWD/s60devicerunconfiguration.h \
    $$PWD/s60devicerunconfigurationwidget.h \
    $$PWD/serialdevicelister.h \
    $$PWD/rvcttoolchain.h \
    $$PWD/s60runconfigbluetoothstarter.h \
    $$PWD/abldparser.h \
    $$PWD/rvctparser.h \
    $$PWD/winscwparser.h
FORMS += $$PWD/s60devicespreferencepane.ui
include(../../../shared/trk/trk.pri)||error("could not include trk.pri")
