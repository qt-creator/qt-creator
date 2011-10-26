include($$PWD/../../../libs/qmljsdebugclient/qmljsdebugclient.pri)

HEADERS += \
    $$PWD/qmlengine.h \
    $$PWD/qmladapter.h \
    $$PWD/qmldebuggerclient.h \
    $$PWD/qmljsprivateapi.h \
    $$PWD/qmlcppengine.h \
    $$PWD/qmljsscriptconsole.h \
    $$PWD/qscriptdebuggerclient.h \
    $$PWD/qmlv8debuggerclient.h \
    $$PWD/interactiveinterpreter.h \
    $$PWD/qmlv8debuggerclientconstants.h

SOURCES += \
    $$PWD/qmlengine.cpp \
    $$PWD/qmladapter.cpp \
    $$PWD/qmldebuggerclient.cpp \
    $$PWD/qmlcppengine.cpp \
    $$PWD/qmljsscriptconsole.cpp \
    $$PWD/qscriptdebuggerclient.cpp \
    $$PWD/qmlv8debuggerclient.cpp \
    $$PWD/interactiveinterpreter.cpp

