#-------------------------------------------------
#
# Project created by QtCreator 2010-02-24T10:21:38
#
#-------------------------------------------------

TARGET = qmldump
QT += declarative
CONFIG += console

TEMPLATE = app

SOURCES += main.cpp \
    qmlstreamwriter.cpp

OTHER_FILES += Info.plist.in
macx {
    info.input = Info.plist.in
    info.output = $$DESTDIR/$${TARGET}.app/Contents/Info.plist
    QMAKE_SUBSTITUTES += info
}

HEADERS += \
    qmlstreamwriter.h
