INCLUDEPATH *= $$PWD

# Input
HEADERS += $$PWD/callback.h \
    $$PWD/trkutils.h \
    $$PWD/trkdevice.h \
    $$PWD/launcher.h \
    $$PWD/bluetoothlistener.h

SOURCES += $$PWD/trkutils.cpp \
    $$PWD/trkdevice.cpp \
    $$PWD/launcher.cpp \
    $$PWD/bluetoothlistener.cpp

contains(QT, gui) {
   HEADERS += $$PWD/bluetoothlistener_gui.h
    SOURCES += $$PWD/bluetoothlistener_gui.cpp
} else {
    message(Trk: Console ...)
}
