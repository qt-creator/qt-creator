TEMPLATE = lib
TARGET = DoNothing

isEmpty(QTC_SOURCE):IDE_SOURCE_TREE=$$PWD/../../../../../
else:IDE_SOURCE_TREE=$$(QTC_SOURCE)

isEmpty(QTC_BUILD):IDE_BUILD_TREE=$$OUT_PWD/../../../../../
else:IDE_BUILD_TREE=$$(QTC_BUILD)

PROVIDER = FooCompanyInc

include($$IDE_SOURCE_TREE/src/qtcreatorplugin.pri)
include($$IDE_SOURCE_TREE/src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

HEADERS = donothingplugin.h
SOURCES = donothingplugin.cpp
OTHER_FILES = DoNothing.pluginspec


