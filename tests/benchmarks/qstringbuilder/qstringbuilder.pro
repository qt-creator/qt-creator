load(qttest_p4)
TEMPLATE = app
TARGET = tst_qstringbuilder
STRINGBUILDERDIR = ../../../src/libs/utils
INCLUDEPATH += $$STRINGBUILDERDIR

QT -= gui

CONFIG += release

# Input
SOURCES += main.cpp $$STRINGBUILDERDIR/qstringbuilder.cpp
