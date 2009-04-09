# Detect presence of "Debugging Tools For Windows"
# in case VS compilers are used.      

win32 {
contains(QMAKE_CXX, cl) {

CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows/sdk"

exists ($$CDB_PATH) {
message("Experimental: Adding support for $$CDB_PATH")

DEFINES+=CDB_ENABLED

CDB_PLATFORM=i386

INCLUDEPATH*=$$CDB_PATH
INCLUDEPATH*=$$PWD

CDB_LIBPATH=$$CDB_PATH/lib/$$CDB_PLATFORM

HEADERS += \
    $$PWD/cdbdebugengine.h \
    $$PWD/cdbdebugengine_p.h \
    $$PWD/cdbdebugeventcallback.h \
    $$PWD/cdbdebugoutput.h \
    $$PWD/cdbsymbolgroupcontext.h \
    $$PWD/cdbstacktracecontext.h \
    $$PWD/cdbbreakpoint.h \
    $$PWD/cdbmodules.h \
    $$PWD/cdbassembler.h

SOURCES += \
    $$PWD/cdbdebugengine.cpp \
    $$PWD/cdbdebugeventcallback.cpp \
    $$PWD/cdbdebugoutput.cpp \
    $$PWD/cdbsymbolgroupcontext.cpp \
    $$PWD/cdbstacktracecontext.cpp \
    $$PWD/cdbbreakpoint.cpp \
    $$PWD/cdbmodules.cpp \
    $$PWD/cdbassembler.cpp

} else {
   message("Debugging Tools for Windows could not be found in $$CDB_PATH")
}
}
}
