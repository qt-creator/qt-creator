CONFIG += testcase

include(../../../qtcreator.pri)
include($$IDE_SOURCE_TREE/src/libs/cplusplus/cplusplus.pri)
include($$IDE_SOURCE_TREE/src/plugins/cpptools/cpptools.pri)

QT += $$QTESTLIB

DEFINES += ICHECK_BUILD ICHECK_APP_BUILD

INCLUDEPATH += $$IDE_SOURCE_TREE/src/libs/cplusplus
INCLUDEPATH += $$IDE_SOURCE_TREE/src/plugins
LIBS += $$IDE_SOURCE_TREE/lib/qtcreator/plugins

HEADERS += ichecklib.h \
           ichecklib_global.h \
           parsemanager.h
SOURCES += ichecklib.cpp \
           parsemanager.cpp \
           tst_icheckbuild.cpp
