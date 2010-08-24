#-------------------------------------------------
#
# Project created by QtCreator 2010-02-24T10:21:38
#
#-------------------------------------------------

TARGET = qmldump
QT += declarative

TEMPLATE = app

SOURCES += main.cpp

include(../../../../qtcreator.pri)
include(../../../private_headers.pri)
DESTDIR = $$IDE_BIN_PATH
include(../../../rpath.pri)

OTHER_FILES += Info.plist
macx:QMAKE_INFO_PLIST = Info.plist
