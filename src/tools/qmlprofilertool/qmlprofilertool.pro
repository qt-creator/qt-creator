include(../../../qtcreator.pri)

TEMPLATE = app
TARGET   = qmlprofiler
DESTDIR = $$IDE_APP_PATH

QT       = core
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



