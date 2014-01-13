QTC_PLUGIN_DEPENDS += core
include(../qttest.pri)

LIBS *= -L$$IDE_PLUGIN_PATH/QtProject

SOURCES += \
    tst_treeviewfind.cpp
