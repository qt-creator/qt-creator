#include(../shared/shared.pri)
QTC_LIB_DEPENDS += utils
include(../../qttest.pri)
#DEFINES+=CPLUSPLUS_BUILD_STATIC_LIB

#DEFINES += Q_PLUGIN_PATH=\"\\\"$$IDE_PLUGIN_PATH\\\"\"

DEFINES += TESTSRCDIR=\\\"$$PWD\\\"
SOURCES += $$PWD/tst_qrcparser.cpp
