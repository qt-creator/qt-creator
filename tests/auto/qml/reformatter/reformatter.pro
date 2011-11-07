include(../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"

include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs.pri)

TARGET = tst_reformatter

SOURCES += \
    tst_reformatter.cpp
