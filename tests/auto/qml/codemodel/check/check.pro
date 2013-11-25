include(../../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"

include($$IDE_SOURCE_TREE/src/libs/utils/utils-lib.pri)
include($$IDE_SOURCE_TREE/src/libs/languageutils/languageutils-lib.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs-lib.pri)

TARGET = tst_codemodel_check

SOURCES += \
    tst_check.cpp
