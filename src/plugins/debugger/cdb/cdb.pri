# Detect presence of "Debugging Tools For Windows"
# in case VS compilers are used.      

win32 {
contains(QMAKE_CXX, cl) {

CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows/sdk"

!exists ($$CDB_PATH) {
  CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x86)/sdk"
}

!exists ($$CDB_PATH) {
  CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows (x64)/sdk"
}

!exists ($$CDB_PATH) {
  CDB_PATH="$$(ProgramFiles)/Debugging Tools For Windows 64-bit/sdk"
}

exists ($$CDB_PATH) {
message("Adding support for $$CDB_PATH")

DEFINES+=CDB_ENABLED

CDB_PLATFORM=i386

INCLUDEPATH*=$$CDB_PATH
INCLUDEPATH*=$$PWD
DEPENDPATH*=$$PWD

CDB_LIBPATH=$$CDB_PATH/lib/$$CDB_PLATFORM

HEADERS += \
    $$PWD/cdbcom.h \
    $$PWD/cdbdebugengine.h \
    $$PWD/cdbdebugengine_p.h \
    $$PWD/cdbdebugeventcallback.h \
    $$PWD/cdbdebugoutput.h \
    $$PWD/cdbsymbolgroupcontext.h \
    $$PWD/cdbsymbolgroupcontext_tpl.h \
    $$PWD/cdbstacktracecontext.h \
    $$PWD/cdbstackframecontext.h \
    $$PWD/cdbbreakpoint.h \
    $$PWD/cdbmodules.h \
    $$PWD/cdbassembler.h \
    $$PWD/cdboptions.h \
    $$PWD/cdboptionspage.h \
    $$PWD/cdbdumperhelper.h \
    $$PWD/cdbsymbolpathlisteditor.h \
    $$PWD/cdbexceptionutils.h

SOURCES += \
    $$PWD/cdbdebugengine.cpp \
    $$PWD/cdbdebugeventcallback.cpp \
    $$PWD/cdbdebugoutput.cpp \
    $$PWD/cdbsymbolgroupcontext.cpp \
    $$PWD/cdbstackframecontext.cpp \
    $$PWD/cdbstacktracecontext.cpp \
    $$PWD/cdbbreakpoint.cpp \
    $$PWD/cdbmodules.cpp \
    $$PWD/cdbassembler.cpp \
    $$PWD/cdboptions.cpp \
    $$PWD/cdboptionspage.cpp \
    $$PWD/cdbdumperhelper.cpp \
    $$PWD/cdbsymbolpathlisteditor.cpp \
    $$PWD/cdbexceptionutils.cpp

FORMS += $$PWD/cdboptionspagewidget.ui

} else {
   message("Debugging Tools for Windows could not be found in $$CDB_PATH")
}
}
}
