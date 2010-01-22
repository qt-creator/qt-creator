# Detect presence of "Debugging Tools For Windows"
# in case VS compilers are used.      

win32 {
contains(QMAKE_CXX, cl) {

CDB_PATH="$$(CDB_PATH)"
isEmpty(CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows/sdk"

!exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x86)/sdk"
!exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x64)/sdk"
!exists($$CDB_PATH):CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows 64-bit/sdk"

exists($$CDB_PATH) {

message("Adding support for $$CDB_PATH")

DEFINES+=CDB_ENABLED

CDB_PLATFORM=i386

INCLUDEPATH*=$$CDB_PATH
CDB_LIBPATH=$$CDB_PATH/lib/$$CDB_PLATFORM

HEADERS += \
    $$PWD/cdbcom.h \
    $$PWD/coreengine.h \
    $$PWD/debugoutputbase.h \
    $$PWD/debugeventcallbackbase.h
SOURCES += \    
    $$PWD/coreengine.cpp \
    $$PWD/debugoutputbase.cpp \
    $$PWD/debugeventcallbackbase.cpp

INCLUDEPATH*=$$PWD
DEPENDPATH*=$$PWD

} else {
   message("Debugging Tools for Windows could not be found in $$CDB_PATH")
   CDB_PATH=""
} # exists($$CDB_PATH)
} # (QMAKE_CXX, cl)
} # win32
