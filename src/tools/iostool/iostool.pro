TARGET = iostool

QT       += core
QT       += gui xml network

CONFIG   += console

# Prevent from popping up in the dock when launched.
# We embed the Info.plist file, so the application doesn't need to
# be a bundle.
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,\"$$PWD/Info.plist\"
CONFIG -= app_bundle

LIBS += -framework CoreFoundation -framework CoreServices -framework IOKit -framework Security -framework SystemConfiguration

TEMPLATE = app

include(../../../qtcreator.pri)

# put into a subdir, so we can deploy a separate qt.conf for it
DESTDIR = $$IDE_LIBEXEC_PATH/ios
include(../../rpath.pri)

SOURCES += main.cpp \
    iosdevicemanager.cpp

HEADERS += \
    iosdevicemanager.h

DISTFILES += Info.plist

target.path = $$INSTALL_LIBEXEC_PATH/ios
INSTALLS += target
