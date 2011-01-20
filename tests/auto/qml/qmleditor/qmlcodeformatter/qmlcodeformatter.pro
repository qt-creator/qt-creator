TEMPLATE = app
CONFIG += qt warn_on console depend_includepath qtestlib testcase
QT += network

SRCDIR = ../../../../../src

#DEFINES += QML_BUILD_STATIC_LIB
#include($$SRCDIR/../qtcreator.pri)
include($$SRCDIR/libs/qmljs/qmljs-lib.pri)
include($$SRCDIR/libs/utils/utils-lib.pri)
include($$SRCDIR/libs/languageutils/languageutils-lib.pri)
#LIBS += -L$$IDE_LIBRARY_PATH

SOURCES += \
    tst_qmlcodeformatter.cpp \
    $$SRCDIR/plugins/qmljstools/qmljsqtstylecodeformatter.cpp \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.cpp

HEADERS += \
    $$SRCDIR/plugins/qmljstools/qmljseditorcodeformatter.h \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.h

INCLUDEPATH += $$SRCDIR/plugins $$SRCDIR/libs
