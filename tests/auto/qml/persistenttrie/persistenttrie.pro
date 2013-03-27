include(../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$PWD\\\"

include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
include($$IDE_SOURCE_TREE/src/libs/languageutils/languageutils.pri)
include($$IDE_SOURCE_TREE/src/libs/qmljs/qmljs.pri)

TARGET = tst_trie_check

HEADERS += tst_testtrie.h

SOURCES += \
    tst_testtrie.cpp

TEMPLATE = app
TARGET = tester
DEFINES += QMLJS_BUILD_DIR

OTHER_FILES += \
    listAll.data \
    intersect.data \
    merge.data \
    completion.data
