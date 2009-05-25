load(qttest_p4)
TEMPLATE = app
TARGET = tst_qstringbuilder
STRINGBUILDERDIR = ../../../src/libs/utils
INCLUDEPATH += $$STRINGBUILDERDIR

QMAKE_CXXFLAGS += -g
QMAKE_CFLAGS += -g

QT -= gui

CONFIG += release

# Input
SOURCES += main.cpp $$STRINGBUILDERDIR/qstringbuilder.cpp
