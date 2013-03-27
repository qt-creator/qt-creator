include(../qttest.pri)

# for shellQuote(). hopefully without side effects ...
isEqual(QT_MAJOR_VERSION, 4):DEFINES += QT_BOOTSTRAPPED
INCLUDEPATH += $$IDE_SOURCE_TREE/src/shared
SOURCES += tst_ioutils.cpp \
    $$IDE_SOURCE_TREE/src/shared/proparser/ioutils.cpp
