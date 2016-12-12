QTC_LIB_DEPENDS += cplusplus utils extensionsystem
include(../../../qttest.pri)
include($$IDE_SOURCE_TREE/src/rpath.pri)
DEFINES += QMLJS_LIBRARY

QT += qml xml
# direct dependency on qmljs for quicker turnaround when editing them
INCLUDEPATH+=$$IDE_SOURCE_TREE/src/libs
INCLUDEPATH+=$$IDE_SOURCE_TREE/src/libs/qmljs
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs-lib.pri)
include($$IDE_SOURCE_TREE/src/libs/languageutils/languageutils-lib.pri)
DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"
LIBS += "-L$$IDE_LIBRARY_PATH"

TARGET = tst_qml_imports_check

SOURCES += \
    tst_importscheck.cpp
