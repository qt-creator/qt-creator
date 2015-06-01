QT += core network testlib
QT -= gui

TEMPLATE = app

CONFIG   += console c++14 testcase
CONFIG   -= app_bundle

TARGET = testlib

include(../../clang.pri)
include(../../clangsource/codemodelbackendclang-source.pri)


LIBS += -L$$OUT_PWD/../codemodelbackendipc/lib/qtcreator -lCodemodelbackendipc


SOURCES += tst_clang.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

INCLUDEPATH *= $$IDE_SOURCE_TREE/src/libs/codemodelbackendipc/source

HEADERS += \
    foo.h

DEFINES += TEST_BASE_DIRECTORY=\\\"$$PWD\\\"
