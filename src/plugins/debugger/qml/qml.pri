include($$PWD/../../../libs/qmljsdebugclient/qmljsdebugclient-lib.pri)

HEADERS += \
    $$PWD/qmlengine.h \
    $$PWD/qmladapter.h \
    $$PWD/qmldebuggerclient.h \
    $$PWD/qmljsprivateapi.h \
    $$PWD/qmlcppengine.h
SOURCES += \
    $$PWD/qmlengine.cpp \
    $$PWD/qmladapter.cpp \
    $$PWD/qmldebuggerclient.cpp \
    $$PWD/qmlcppengine.cpp
