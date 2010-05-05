TARGET = %PluginName%
TEMPLATE = lib

DEFINES += %PluginName:u%_LIBRARY

SOURCES += %PluginName:l%.cpp

HEADERS += %PluginName:l%.h\
        %PluginName:l%_global.h

OTHER_FILES = %PluginName%.pluginspec

QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=%QtCreatorSources%

IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=%QtCreatorBuild%
PROVIDER = %VendorName%

include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
include($$QTCREATOR_SOURCES/src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia
