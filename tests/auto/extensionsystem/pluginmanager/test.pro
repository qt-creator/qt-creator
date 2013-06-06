TARGET = pluginmanager

# Input

QTC_LIB_DEPENDS += extensionsystem
include(../../qttest.pri)

SOURCES += tst_pluginmanager.cpp

OTHER_FILES = $$PWD/plugins/otherplugin.xml \
    $$PWD/plugins/plugin1.xml \
    $$PWD/plugins/myplug/myplug.xml

COPYDIR = $$OUT_PWD
COPYFILES = $$OTHER_FILES
include(../copy.pri)

DEFINES += "PLUGINMANAGER_TESTS_DIR=\\\"$$OUT_PWD\\\""
