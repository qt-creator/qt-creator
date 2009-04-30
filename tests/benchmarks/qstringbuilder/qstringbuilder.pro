load(qttest_p4)
TEMPLATE = app
TARGET = tst_qstringbuilder
DEPENDPATH += .
INCLUDEPATH += .

QT -= gui

CONFIG += release

# Input
SOURCES += main.cpp
