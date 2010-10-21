include(../../qttest.pri)
include(../shared/shared.pri)

SRCDIR = ../../../../src

SOURCES += \
    tst_codeformatter.cpp \
    $$SRCDIR/plugins/cpptools/cppcodeformatter.cpp \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.cpp

HEADERS += \
    $$SRCDIR/plugins/cpptools/cppcodeformatter.h \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.h

INCLUDEPATH += $$SRCDIR/plugins $$SRCDIR/libs
