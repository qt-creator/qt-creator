QTC_LIB_DEPENDS += qmljs
QTC_PLUGIN_DEPENDS += qmljstools
include(../../../qttest.pri)

SRCDIR = $$IDE_SOURCE_TREE/src

SOURCES += \
    tst_qmlcodeformatter.cpp
