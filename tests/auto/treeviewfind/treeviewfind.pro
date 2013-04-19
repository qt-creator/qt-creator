QTC_PLUGIN_DEPENDS += find
include(../qttest.pri)

LIBS *= -L$$IDE_PLUGIN_PATH/QtProject

SOURCES += \
    tst_treeviewfind.cpp
