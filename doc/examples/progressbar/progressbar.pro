QTC_SOURCE = C:/Work/QtCreator
QTC_BUILD  = C:/Work/QtCreator/build
DEFINES += FIND_LIBRARY

TEMPLATE = lib
TARGET = ProgressBar
IDE_SOURCE_TREE = $$QTC_SOURCE
IDE_BUILD_TREE = $$QTC_BUILD

PROVIDER = FooCompanyInc


include($$QTC_SOURCE/src/libs/extensionsystem/extensionsystem.pri)
include($$QTC_SOURCE/src/qtcreatorplugin.pri)
include($$QTC_SOURCE/src/plugins/coreplugin/coreplugin.pri)
include($$QTC_SOURCE/src/libs/utils/utils.pri)
include($$QTC_SOURCE/src/plugins/projectexplorer/projectexplorer.pri)
include($$QTC_SOURCE/src/plugins/find/find.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

HEADERS = progressbarplugin.h \
    headerfilterprogress.h
SOURCES = progressbarplugin.cpp \
    headerfilteprogress.cpp
    
OTHER_FILES = ProgressBar.pluginspec

