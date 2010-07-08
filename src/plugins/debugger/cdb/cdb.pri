include(cdbcore.pri)

!isEmpty(CDB_PATH) {

HEADERS += \
    $$PWD/cdbengine.h \
    $$PWD/cdbengine_p.h \
    $$PWD/cdbdebugeventcallback.h \
    $$PWD/cdbdebugoutput.h \
    $$PWD/cdbsymbolgroupcontext.h \
    $$PWD/cdbsymbolgroupcontext_tpl.h \
    $$PWD/cdbstacktracecontext.h \
    $$PWD/cdbbreakpoint.h \
    $$PWD/cdbmodules.h \
    $$PWD/cdbassembler.h \
    $$PWD/cdboptions.h \
    $$PWD/cdboptionspage.h \
    $$PWD/cdbdumperhelper.h \
    $$PWD/cdbsymbolpathlisteditor.h \
    $$PWD/cdbexceptionutils.h

SOURCES += \
    $$PWD/cdbengine.cpp \
    $$PWD/cdbdebugeventcallback.cpp \
    $$PWD/cdbdebugoutput.cpp \
    $$PWD/cdbsymbolgroupcontext.cpp \
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
}


# Compile test on non-Windows platforms
isEmpty(CDB_PATH) {
false {

HEADERS += \
    $$PWD/cdbcom.h \
    $$PWD/coreengine.h \
    $$PWD/debugoutputbase.h \
    $$PWD/debugeventcallbackbase.h \
    $$PWD/symbolgroupcontext.h \
    $$PWD/stacktracecontext.h \
    $$PWD/breakpoint.h

HEADERS += \
    $$PWD/cdbengine.h \
    $$PWD/cdbengine_p.h \
    $$PWD/cdbdebugeventcallback.h \
    $$PWD/cdbdebugoutput.h \
    $$PWD/cdbsymbolgroupcontext.h \
    $$PWD/cdbsymbolgroupcontext_tpl.h \
    $$PWD/cdbstacktracecontext.h \
    $$PWD/cdbbreakpoint.h \
    $$PWD/cdbmodules.h \
    $$PWD/cdbassembler.h \
    $$PWD/cdboptions.h \
    $$PWD/cdboptionspage.h \
    $$PWD/cdbdumperhelper.h \
    $$PWD/cdbsymbolpathlisteditor.h \
    $$PWD/cdbexceptionutils.h

SOURCES += \
#    $$PWD/coreengine.cpp \
#    $$PWD/debugoutputbase.cpp \
#    $$PWD/debugeventcallbackbase.cpp \
#    $$PWD/symbolgroupcontext.cpp \
#    $$PWD/stacktracecontext.cpp \
#    $$PWD/breakpoint.cpp

SOURCES += \
#    $$PWD/cdbengine.cpp \
#    $$PWD/cdbdebugeventcallback.cpp \
    $$PWD/cdbdebugoutput.cpp \
#    $$PWD/cdbsymbolgroupcontext.cpp \
    $$PWD/cdbstacktracecontext.cpp \
    $$PWD/cdbbreakpoint.cpp \
#    $$PWD/cdbmodules.cpp \
    $$PWD/cdbassembler.cpp \
    $$PWD/cdboptions.cpp \
    $$PWD/cdboptionspage.cpp \
#    $$PWD/cdbdumperhelper.cpp \
    $$PWD/cdbsymbolpathlisteditor.cpp \
#    $$PWD/cdbexceptionutils.cpp

FORMS += $$PWD/cdboptionspagewidget.ui
INCLUDEPATH*=$$PWD
DEPENDPATH*=$$PWD
}
}

