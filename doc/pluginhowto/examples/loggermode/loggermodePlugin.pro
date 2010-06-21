QTC_SOURCE = C:/Work/QtCreator
QTC_BUILD = C:/Work/QtCreator/build
TEMPLATE = lib
TARGET = loggermode
IDE_SOURCE_TREE = $$QTC_SOURCE
IDE_BUILD_TREE = $$QTC_BUILD
PROVIDER = FooCompanyInc

include($$QTC_SOURCE/src/qtcreatorplugin.pri)
include($$QTC_SOURCE/src/plugins/coreplugin/coreplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

HEADERS = loggermodePlugin.h \
    loggermode.h \
    loggermodewidget.h
SOURCES = loggermodePlugin.cpp \
    loggermode.cpp \
    loggermodewidget.cpp
OTHER_FILES = LoggerMode.pluginspec
