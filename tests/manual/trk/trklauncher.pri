DEFINES += DEBUG_TRK=0
DEBUGGERHOME = ../../../src/plugins/debugger/symbian

INCLUDEPATH *= $$DEBUGGERHOME

SOURCES += \
    $$DEBUGGERHOME/trkutils.cpp \
    $$PWD/trkdevice.cpp \
    $$PWD/launcher.cpp \

HEADERS += \
    $$DEBUGGERHOME/trkutils.h \
    $$DEBUGGERHOME/trkfunctor.h \
    $$PWD/trkdevice.h \
    $$PWD/launcher.h
