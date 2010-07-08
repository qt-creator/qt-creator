TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT += testlib

include(../shared/shared.pri)

SRCDIR = ../../../../src

SOURCES += \
    tst_codeformatter.cpp \
    $$SRCDIR/plugins/cpptools/cppcodeformatter.cpp \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.cpp

HEADERS += \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.h

INCLUDEPATH += $$SRCDIR/plugins $$SRCDIR/libs

TARGET=tst_$$TARGET
