include(cdbcore.pri)

!isEmpty(CDB_PATH) {

HEADERS += \
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

LIBS+=-lpsapi
}
