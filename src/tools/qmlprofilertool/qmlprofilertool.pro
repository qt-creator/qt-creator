QT       = core
include(../../../qtcreator.pri)
include(../../rpath.pri)

TEMPLATE = app
TARGET   = qmlprofiler
DESTDIR = $$IDE_BIN_PATH

CONFIG   += console
CONFIG   -= app_bundle

include(../../shared/symbianutils/symbianutils.pri)
include(../../libs/qmljsdebugclient/qmljsdebugclient-lib.pri)

INCLUDEPATH += ../../libs/qmljsdebugclient

SOURCES += main.cpp \
    qmlprofilerapplication.cpp \
    commandlistener.cpp

HEADERS += \
    qmlprofilerapplication.h \
    commandlistener.h \
    constants.h



