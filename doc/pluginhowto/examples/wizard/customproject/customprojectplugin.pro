#QTC_SOURCE = C:/Work/QtCreator
#QTC_BUILD = C:/Work/QtCreator/build
QTC_SOURCE = ../../../../..
QTC_BUILD = ../../../../..
TEMPLATE = lib
TARGET = CustomProject
IDE_SOURCE_TREE = $$QTC_SOURCE
IDE_BUILD_TREE = $$QTC_BUILD
PROVIDER = FooCompanyInc

include($$QTC_SOURCE/src/qtcreatorplugin.pri)
include($$QTC_SOURCE/src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

HEADERS = customprojectplugin.h \
    customprojectwizard.h
SOURCES = customprojectplugin.cpp \
    customprojectwizard.cpp
OTHER_FILES = CustomProject.pluginspec
