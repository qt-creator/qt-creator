#-------------------------------------------------
#
# Project created by QtCreator 2015-01-14T17:00:27
#
#-------------------------------------------------


TARGET = tst_codemodelbackendprocess

include(../codemodelbackendprocesstestcommon.pri)

SOURCES += tst_codemodelbackendprocess.cpp \
    clientdummy.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

INCLUDEPATH *= $$IDE_SOURCE_TREE/src/libs/codemodelbackendipc/source

HEADERS += \
    clientdummy.h \
    foo.h

DEFINES += TEST_BASE_DIRECTORY=\\\"$$PWD\\\"
