TARGET = qtcdebugger
TEMPLATE = app
CONFIG += console
SOURCES += main.cpp

LIBS *= -lpsapi
# PS API and registry functions
contains(QMAKE_CXX, cl) {
    LIBS *= -ladvapi32
}

DESTDIR=..\..\..\bin
