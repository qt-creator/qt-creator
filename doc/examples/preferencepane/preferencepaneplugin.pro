QTC_SOURCE = C:/Work/QtCreator
QTC_BUILD = C:/Work/QtCreator/build
TEMPLATE = lib
TARGET = PreferencePane
IDE_SOURCE_TREE = $$QTC_SOURCE
IDE_BUILD_TREE = $$QTC_BUILD

PROVIDER = FooCompanyInc

include($$QTC_SOURCE/src/qtcreatorplugin.pri)
include($$QTC_SOURCE/src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

HEADERS = preferencepaneplugin.h \
    modifiedfilelistwidget.h \
    modifiedfilelister.h
SOURCES = preferencepaneplugin.cpp \
    modifiedfilelistwidget.cpp \
    modifiedfilelister.cpp
OTHER_FILES = PreferencePane.pluginspec
