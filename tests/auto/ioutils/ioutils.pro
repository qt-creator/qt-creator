include(../qttest.pri)

DEFINES += QT_BOOTSTRAPPED # for shellQuote(). hopefully without side effects ...
INCLUDEPATH += $$IDE_SOURCE_TREE/src/shared
SOURCES += tst_ioutils.cpp \
    $$IDE_SOURCE_TREE/src/shared/proparser/ioutils.cpp
