TEMPLATE = app
CONFIG += qt warn_on console depend_includepath
QT += testlib network

SRCDIR = ../../../../../src

#DEFINES += QML_BUILD_STATIC_LIB
#include($$SRCDIR/../qtcreator.pri)
include($$SRCDIR/libs/qmljs/qmljs-lib.pri)
include($$SRCDIR/libs/utils/utils-lib.pri)
#LIBS += -L$$IDE_LIBRARY_PATH

SOURCES += \
    tst_codeformatter.cpp \
    $$SRCDIR/plugins/qmljseditor/qmljseditorcodeformatter.cpp \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.cpp

HEADERS += \
    $$SRCDIR/plugins/qmljseditor/qmljseditorcodeformatter.h \
    $$SRCDIR/plugins/texteditor/basetextdocumentlayout.h \

INCLUDEPATH += $$SRCDIR/plugins $$SRCDIR/libs

TARGET=tst_$$TARGET
