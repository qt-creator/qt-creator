TARGET = iosTool

QT       += core
QT       += gui xml

CONFIG   += console

# Prevent from popping up in the dock when launched.
# We embed the Info.plist file, so the application doesn't need to
# be a bundle.
QMAKE_LFLAGS += -sectcreate __TEXT __info_plist \"$$PWD/Info.plist\"
CONFIG -= app_bundle

LIBS += -framework CoreFoundation -framework CoreServices -framework IOKit -lssl -lbz2 -framework Security -framework SystemConfiguration

TEMPLATE = app

include(../../../qtcreator.pri)

DESTDIR = $$IDE_BIN_PATH
include(../../rpath.pri)

SOURCES += main.cpp \
    iosdevicemanager.cpp

HEADERS += \
    iosdevicemanager.h
