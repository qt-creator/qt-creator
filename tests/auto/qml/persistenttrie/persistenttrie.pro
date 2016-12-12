QTC_LIB_DEPENDS += qmljs
include(../../qttest.pri)

DEFINES+=QTCREATORDIR=\\\"$$IDE_SOURCE_TREE\\\"
DEFINES+=TESTSRCDIR=\\\"$$_PRO_FILE_PWD_\\\"

TARGET = tst_trie_check

HEADERS += tst_testtrie.h

SOURCES += \
    tst_testtrie.cpp

TEMPLATE = app
DEFINES += QMLJS_LIBRARY

DISTFILES += \
    listAll.data \
    intersect.data \
    merge.data \
    completion.data
