win32 {
# ---- Detect Debugging Tools For Windows

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
    $$PWD/cdbdebugoutput.h

SOURCES += \
    $$PWD/cdbdebugengine.cpp \
    $$PWD/cdbdebugeventcallback.cpp \
    $$PWD/cdbdebugoutput.cpp
} else {
   error("Debugging Tools for Windows could not be found in $$CDB_PATH")
}
}
