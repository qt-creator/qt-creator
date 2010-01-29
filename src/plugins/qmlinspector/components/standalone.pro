DESTDIR = ../../../bin
TARGET = qmldebugger

include(qmldebugger.pri)

HEADERS += $$PWD/qmldebugger.h \
           $$PWD/engine.h

SOURCES += $$PWD/qmldebugger.cpp \
           $$PWD/engine.cpp \
           $$PWD/main.cpp

RESOURCES += $$PWD/qmldebugger.qrc
OTHER_FILES += $$PWD/engines.qml

target.path=$$[QT_INSTALL_BINS]
INSTALLS += target

CONFIG += console
