include(../../../qttest.pri)

SRCDIR = ../../../../../src

include($$SRCDIR/libs/qmljs/qmljs-lib.pri)
include($$SRCDIR/libs/utils/utils-lib.pri)
include($$SRCDIR/libs/languageutils/languageutils-lib.pri)

SOURCES += \
    tst_qmlcodeformatter.cpp \
    $$SRCDIR/plugins/qmljstools/qmljsqtstylecodeformatter.cpp \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.cpp \
    $$SRCDIR/plugins/texteditor/itextmark.cpp

HEADERS += \
    $$SRCDIR/plugins/qmljstools/qmljsqtstylecodeformatter.h \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.h \
    $$SRCDIR/plugins/texteditor/itextmark.h
