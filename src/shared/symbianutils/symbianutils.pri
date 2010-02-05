INCLUDEPATH *= $$PWD

# Input
HEADERS += $$PWD/symbianutils_global.h \
    $$PWD/callback.h \
    $$PWD/trkutils.h \
    $$PWD/trkutils_p.h \
    $$PWD/trkdevice.h \
    $$PWD/launcher.h \
    $$PWD/bluetoothlistener.h \
    $$PWD/communicationstarter.h \
    $$PWD/symbiandevicemanager.h

SOURCES += $$PWD/trkutils.cpp \
    $$PWD/trkdevice.cpp \
    $$PWD/launcher.cpp \
    $$PWD/bluetoothlistener.cpp \
    $$PWD/communicationstarter.cpp \
    $$PWD/symbiandevicemanager.cpp

# Tests/trklauncher is a console application
contains(QT, gui) {
   HEADERS += $$PWD/bluetoothlistener_gui.h
    SOURCES += $$PWD/bluetoothlistener_gui.cpp
} else {
    message(Trk: Console ...)
}
