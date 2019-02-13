#include(../shared/shared.pri)
QTC_LIB_DEPENDS += utils
include(../../qttest.pri)
#DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB
include($$IDE_SOURCE_TREE/src/rpath.pri)

#DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH\\\"\"

DEFINES += TESTSRCDIR=\\\"$$PWD\\\"
SOURCES += $$PWD/tst_qrcparser.cpp
