CONFIG   += console

QT       += core
QT       += gui

CONFIG -= app_bundle

include(../../../../qtcreator.pri)

# Prevent from popping up in the dock when launched.
# We embed the Info.plist file, so the application doesn't need to
# be a bundle.
QMAKE_LFLAGS += -Wl,-sectcreate,__TEXT,__info_plist,\"$$PWD/Info.plist\" \
  -fobjc-link-runtime

LIBS += \
  -framework Foundation \
  -framework CoreServices \
  -framework ApplicationServices \
  -framework CoreFoundation \
  -F/System/Library/PrivateFrameworks \
  -framework IOKit -framework AppKit

iPhoneSimulatorRemoteClientDirectLinking {
  LIBS += \
    -F/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks \
    -F/Applications/Xcode.app/Contents/OtherFrameworks
  LIBS += \
    -framework iPhoneSimulatorRemoteClient
  QMAKE_RPATHDIR += /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks \
    /Applications/Xcode.app/Contents/OtherFrameworks
    /System/Library/PrivateFrameworks \
}

TEMPLATE = app

# put into a subdir, so we can deploy a separate qt.conf for it
DESTDIR = $$IDE_LIBEXEC_PATH/ios
include(../../../rpath.pri)

OBJECTIVE_SOURCES += \
  main.mm \
  nsprintf.mm \
  nsstringexpandpath.mm \
  iphonesimulator.mm

HEADERS += \
  iphonesimulator.h \
  nsprintf.h \
  nsstringexpandpath.h \
  version.h \
  iphonesimulatorremoteclient/iphonesimulatorremoteclient.h

DISTFILES = IOSSIM_LICENSE \
  Info.plist
