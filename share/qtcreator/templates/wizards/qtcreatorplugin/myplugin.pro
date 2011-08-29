TARGET = %PluginName%
TEMPLATE = lib

DEFINES += %PluginName:u%_LIBRARY

# %PluginName% files

SOURCES += %PluginName:l%plugin.cpp

HEADERS += %PluginName:l%plugin.h\
        %PluginName:l%_global.h\
        %PluginName:l%constants.h

OTHER_FILES = %PluginName%.pluginspec


# Qt Creator linking

## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=%QtCreatorSources%

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=%QtCreatorBuild%

## uncomment to build plugin into user config directory
## <localappdata>/plugins/<ideversion>
##    where <localappdata> is e.g.
##    <drive>:\Users\<username>\AppData\Local\Nokia\QtCreator on Windows Vista and later
##    $XDG_DATA_HOME/Nokia/QtCreator or ~/.local/share/Nokia/QtCreator on Linux
##    ~/Library/Application Support/Nokia/QtCreator on Mac
%DestDir%USE_USER_DESTDIR = yes

PROVIDER = %VendorName%

include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
include($$QTCREATOR_SOURCES/src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia
