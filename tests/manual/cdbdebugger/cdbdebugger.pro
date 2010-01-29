TEMPLATE = app
TARGET =
DEPENDPATH += .
INCLUDEPATH += .

DEFINES +=

LIBS += Dbghelp.lib dbgeng.lib

HEADERS += mainwindow.h \
           debugger.h \
           outputcallback.h \
           windbgeventcallback.h \
           windbgthread.h
FORMS += mainwindow.ui
SOURCES += main.cpp mainwindow.cpp \
           debugger.cpp \
           outputcallback.cpp \
           windbgeventcallback.cpp \
           windbgthread.cpp
