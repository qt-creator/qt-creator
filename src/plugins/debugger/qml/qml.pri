include($$PWD/../../../libs/qmljsdebugclient/qmljsdebugclient.pri)
include($$PWD/../../../shared/json/json.pri)
DEFINES += JSON_INCLUDE_PRI

HEADERS += \
    $$PWD/qmlengine.h \
    $$PWD/qmladapter.h \
    $$PWD/qmldebuggerclient.h \
    $$PWD/qmljsprivateapi.h \
    $$PWD/qmlcppengine.h \
    $$PWD/scriptconsole.h \
    $$PWD/qscriptdebuggerclient.h \
    $$PWD/qmlv8debuggerclient.h

SOURCES += \
    $$PWD/qmlengine.cpp \
    $$PWD/qmladapter.cpp \
    $$PWD/qmldebuggerclient.cpp \
    $$PWD/qmlcppengine.cpp \
    $$PWD/scriptconsole.cpp \
    $$PWD/qscriptdebuggerclient.cpp \
    $$PWD/qmlv8debuggerclient.cpp
