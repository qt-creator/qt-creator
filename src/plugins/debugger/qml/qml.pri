include($$PWD/../../../libs/qmldebug/qmldebug.pri)

HEADERS += \
    $$PWD/qmlengine.h \
    $$PWD/qmladapter.h \
    $$PWD/baseqmldebuggerclient.h \
    $$PWD/qmljsprivateapi.h \
    $$PWD/qmlcppengine.h \
    $$PWD/qscriptdebuggerclient.h \
    $$PWD/qmlv8debuggerclient.h \
    $$PWD/interactiveinterpreter.h \
    $$PWD/qmlv8debuggerclientconstants.h

SOURCES += \
    $$PWD/qmlengine.cpp \
    $$PWD/qmladapter.cpp \
    $$PWD/baseqmldebuggerclient.cpp \
    $$PWD/qmlcppengine.cpp \
    $$PWD/qscriptdebuggerclient.cpp \
    $$PWD/qmlv8debuggerclient.cpp \
    $$PWD/interactiveinterpreter.cpp
