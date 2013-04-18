DEFINES += %PluginName:u%_LIBRARY

# %PluginName% files

SOURCES += %PluginName:l%plugin.%CppSourceSuffix%

HEADERS += %PluginName:l%plugin.%CppHeaderSuffix% \
        %PluginName:l%_global.%CppHeaderSuffix% \
        %PluginName:l%constants.%CppHeaderSuffix%

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
##    "%LOCALAPPDATA%\QtProject\qtcreator" on Windows Vista and later
##    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
##    "~/Library/Application Support/QtProject/Qt Creator" on Mac
%DestDir%USE_USER_DESTDIR = yes

PROVIDER = %VendorName%

include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
