QT = core network

msvc: QTC_LIB_DEPENDS += utils
include(../qttest.pri)

DEBUGGERDIR = $$IDE_SOURCE_TREE/src/plugins/debugger
UTILSDIR = $$IDE_SOURCE_TREE/src/libs/utils

INCLUDEPATH += $$DEBUGGERDIR

SOURCES += \
    tst_gdb.cpp \
    $$DEBUGGERDIR/debuggerprotocol.cpp

HEADERS += \
    $$DEBUGGERDIR/debuggerprotocol.h

!msvc {
    SOURCES += \
        $$UTILSDIR/environment.cpp \
        $$UTILSDIR/fileutils.cpp \
        $$UTILSDIR/hostosinfo.cpp \
        $$UTILSDIR/namevaluedictionary.cpp \
        $$UTILSDIR/namevalueitem.cpp \
        $$UTILSDIR/qtcassert.cpp \
        $$UTILSDIR/qtcprocess.cpp \
        $$UTILSDIR/processhandle.cpp \
        $$UTILSDIR/savefile.cpp \

    HEADERS += \
        $$UTILSDIR/environment.h \
        $$UTILSDIR/fileutils.h \
        $$UTILSDIR/hostosinfo.h \
        $$UTILSDIR/namevaluedictionary.h \
        $$UTILSDIR/namevalueitem.h \
        $$UTILSDIR/qtcassert.h \
        $$UTILSDIR/qtcprocess.h \
        $$UTILSDIR/processhandle.h \
        $$UTILSDIR/savefile.h \

    macos: {
         HEADERS += $$UTILSDIR/fileutils_mac.h
         OBJECTIVE_SOURCES += $$UTILSDIR/fileutils_mac.mm
         LIBS += -framework Foundation
    }
}
