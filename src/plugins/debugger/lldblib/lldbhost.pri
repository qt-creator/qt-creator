WITH_LLDB = $$(WITH_LLDB)

HEADERS += $$PWD/ipcenginehost.h \
           $$PWD/lldbenginehost.h

SOURCES +=  $$PWD/ipcenginehost.cpp \
            $$PWD/lldbenginehost.cpp

INCLUDEPATH+=

FORMS +=

RESOURCES +=

!isEmpty(WITH_LLDB) {
    DEFINES += WITH_LLDB
    HEADERS += $$PWD/lldboptionspage.h
    SOURCES += $$PWD/lldboptionspage.cpp
    FORMS += $$PWD/lldboptionspagewidget.ui
}
