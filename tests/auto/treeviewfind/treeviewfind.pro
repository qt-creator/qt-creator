QTC_PLUGIN_DEPENDS += coreplugin
include(../qttest.pri)

LIBS *= -L$$IDE_PLUGIN_PATH

SOURCES += \
    tst_treeviewfind.cpp
